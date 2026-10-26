// Drive MemIOCallback / MemReadIOCallback with a byte-coded script of write/read/seek operations.
#include "fz_schema.h"

#include <algorithm>
#include <cstdint>
#include <vector>

static std::uint64_t g_sink;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
  if (size < 3)
    return -1;

  MemIOCallback mem(1u + data[0]);
  MemReadIOCallback src(data, size);
  std::size_t i = 1;
  unsigned steps = 0;

  while (i + 2 <= size && steps++ < 512) {
    const unsigned op = data[i] % 6;
    const unsigned arg = data[i + 1];
    i += 2;
    switch (op) {
      case 0: {
        const std::size_t n = std::min<std::size_t>(arg, size - i);
        g_sink += mem.write(data + i, n);
        i += n;
        break;
      }
      case 1: {
        std::vector<std::uint8_t> buf(arg * 4u + 1u);
        g_sink += mem.read(buf.data(), buf.size());
        break;
      }
      case 2: {
        // Seeking outside [0, size] makes MemIOCallback::read()/write() index out of bounds
        // (upstream bug, see PR notes); stay inside the buffer so the fuzzer reaches the I/O paths.
        std::int64_t off = static_cast<std::int8_t>(arg) * 16;
        if (i < size)
          off += static_cast<std::int64_t>(data[i++]) - 128;
        const std::int64_t total = static_cast<std::int64_t>(mem.GetDataBufferSize());
        const seek_mode mode = static_cast<seek_mode>(arg % 3);
        const std::int64_t base = mode == seek_beginning ? 0
                                : mode == seek_current   ? static_cast<std::int64_t>(mem.getFilePointer())
                                                         : total;
        const std::int64_t target = std::max<std::int64_t>(0, std::min<std::int64_t>(total, base + off));
        mem.setFilePointer(target, seek_beginning);
        break;
      }
      case 3:
        // write(IOCallback&) overwrites the buffer's total size instead of extending it; only an
        // empty buffer is within its contract.
        if (mem.getFilePointer() == 0 && mem.GetDataBufferSize() == 0) {
          try {
            g_sink += mem.write(src, arg * 8u);
          } catch (...) {
          }
        }
        break;
      case 4: {
        std::vector<std::uint8_t> buf(arg + 1u);
        try {
          mem.readFully(buf.data(), buf.size());
        } catch (...) {
        }
        break;
      }
      default: {
        MemReadIOCallback twin(mem.GetDataBuffer(), mem.GetDataBufferSize());
        twin.setFilePointer(static_cast<std::int8_t>(arg), static_cast<seek_mode>(arg % 3));
        std::vector<std::uint8_t> buf(arg + 1u);
        g_sink += twin.read(buf.data(), buf.size());
        break;
      }
    }
    g_sink += mem.getFilePointer() + mem.GetDataBufferSize();
  }
  return 0;
}
