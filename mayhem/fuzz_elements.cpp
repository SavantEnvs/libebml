// One element of each value type: frame the fuzzer's payload with a valid header, then ReadData,
// read the value back and render it.
#include "fz_schema.h"

#include <cstdint>
#include <exception>
#include <vector>

static std::uint64_t g_sink;

template <class T>
static void Exercise(const std::uint8_t *payload, std::size_t len)
{
  std::vector<std::uint8_t> buf(4 + 8 + len);
  const EbmlId id = EBML_ID(T);
  id.Fill(buf.data());
  std::size_t pos = id.GetLength();
  const int sl = CodedValueLength(len, static_cast<int>(CodedSizeLength(len, 0)), buf.data() + pos);
  pos += static_cast<std::size_t>(sl);
  for (std::size_t i = 0; i < len; i++)
    buf[pos + i] = payload[i];
  pos += len;

  MemReadIOCallback io(buf.data(), pos);
  EbmlElement *el = EbmlElement::FindNextID(io, EBML_INFO(T), pos);
  if (el == nullptr)
    return;

  if (el->ValidateSize()) {
    try {
      el->ReadData(io, SCOPE_ALL_DATA);
    } catch (const std::exception &) {
    }
    g_sink += el->ValueIsSet();

    if (auto *u = dynamic_cast<EbmlUInteger *>(el))
      g_sink += static_cast<std::uint64_t>(*u);
    else if (auto *s = dynamic_cast<EbmlSInteger *>(el))
      g_sink += static_cast<std::uint64_t>(static_cast<std::int64_t>(*s));
    else if (auto *f = dynamic_cast<EbmlFloat *>(el))
      g_sink += static_cast<std::uint64_t>(static_cast<double>(*f) > 0.0);
    else if (auto *d = dynamic_cast<EbmlDate *>(el))
      g_sink += static_cast<std::uint64_t>(d->GetEpochDate());
    else if (auto *us = dynamic_cast<EbmlUnicodeString *>(el))
      g_sink += us->GetValueUTF8().size();
    else if (auto *str = dynamic_cast<EbmlString *>(el))
      g_sink += static_cast<const std::string &>(*str).size();
    else if (auto *b = dynamic_cast<EbmlBinary *>(el))
      g_sink += b->GetSize();

    if (el->ValueIsSet()) {
      MemIOCallback out;
      el->UpdateSize(EbmlElement::WriteAll, true);
      el->Render(out, EbmlElement::WriteAll);
      g_sink += out.GetDataBufferSize();
    }
  }
  delete el;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
  if (size < 1)
    return -1;
  const std::uint8_t *payload = data + 1;
  const std::size_t len = size - 1;
  switch (data[0] % 7) {
    case 0: Exercise<FzUInt>(payload, len); break;
    case 1: Exercise<FzSInt>(payload, len); break;
    case 2: Exercise<FzStr>(payload, len); break;
    case 3: Exercise<FzUni>(payload, len); break;
    case 4: Exercise<FzFloat>(payload, len); break;
    case 5: Exercise<FzDate>(payload, len); break;
    default: Exercise<FzBin>(payload, len); break;
  }
  return 0;
}
