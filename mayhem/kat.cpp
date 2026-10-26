// Known-answer probe for mayhem/test.sh: prints values computed by libebml for fixed inputs.
#include "fz_schema.h"

#include <cstdint>
#include <cstdio>
#include <string>

static void Hex(const char *name, const void *p, std::size_t n)
{
  std::printf("KAT %s=", name);
  for (std::size_t i = 0; i < n; i++)
    std::printf("%02x", static_cast<const std::uint8_t *>(p)[i]);
  std::printf("\n");
}

template <class T>
static T *Reparse(MemIOCallback &mem, EbmlStream &stream, const EbmlCallbacks &info)
{
  mem.setFilePointer(0);
  auto *el = static_cast<T *>(stream.FindNextID(info, mem.GetDataBufferSize()));
  if (el != nullptr) {
    int upper = 0;
    EbmlElement *found = nullptr;
    el->Read(stream, EBML_CONTEXT(el), upper, found, true);
  }
  return el;
}

int main()
{
  // CRC-32 of "123456789"
  {
    EbmlCrc32 crc;
    crc.FillCRC32(reinterpret_cast<const binary *>("123456789"), 9);
    std::printf("KAT crc32=%08x\n", crc.GetCrc32());
  }

  // Coded sizes and IDs
  {
    binary buf[8] = {};
    std::printf("KAT sizelen=%u,%u,%u,%u\n", CodedSizeLength(0, 0), CodedSizeLength(126, 0),
                CodedSizeLength(127, 0), CodedSizeLength(16382, 0));
    const int n = CodedValueLength(300, 2, buf);
    Hex("codedvalue300", buf, static_cast<std::size_t>(n));
    std::uint32_t len = 2;
    std::uint64_t unknown = 0;
    std::printf("KAT readcoded=%llu\n", static_cast<unsigned long long>(ReadCodedSizeValue(buf, len, unknown)));
  }

  // Render a tree covering every value type
  MemIOCallback mem;
  FzRoot root;
  GetChild<FzUInt>(root).SetValue(300);
  GetChild<FzSInt>(root).SetValue(-2);
  GetChild<FzStr>(root).SetValue("abc");
  GetChild<FzUni>(root).SetValueUTF8("h\xc3\xa9llo");
  GetChild<FzFloat>(root).SetValue(1.5);
  GetChild<FzDate>(root).SetEpochDate(0);
  const binary raw[3] = {0xDE, 0xAD, 0xBE};
  GetChild<FzBin>(root).CopyBuffer(raw, 3);
  auto &sub = GetChild<FzSub>(root);
  GetChild<FzSubUInt>(sub).SetValue(7);
  GetChild<FzDeepStr>(GetChild<FzDeep>(sub)).SetValue("z");
  const auto written = root.Render(mem, EbmlElement::WriteAll);
  std::printf("KAT rendered=%llu\n", static_cast<unsigned long long>(written));
  Hex("tree", mem.GetDataBuffer(), static_cast<std::size_t>(mem.GetDataBufferSize()));

  // Parse it back
  EbmlStream stream(mem);
  auto *back = Reparse<FzRoot>(mem, stream, EBML_INFO(FzRoot));
  if (back == nullptr) {
    std::printf("KAT reparse=failed\n");
    return 1;
  }
  std::printf("KAT children=%zu\n", back->GetElementList().size());
  std::printf("KAT uint=%llu\n", static_cast<unsigned long long>(static_cast<std::uint64_t>(GetChild<FzUInt>(*back))));
  std::printf("KAT sint=%lld\n", static_cast<long long>(static_cast<std::int64_t>(GetChild<FzSInt>(*back))));
  std::printf("KAT str=%s\n", static_cast<const std::string &>(GetChild<FzStr>(*back)).c_str());
  std::printf("KAT uni=%s\n", GetChild<FzUni>(*back).GetValueUTF8().c_str());
  std::printf("KAT float=%.2f\n", static_cast<double>(GetChild<FzFloat>(*back)));
  std::printf("KAT date=%lld\n", static_cast<long long>(GetChild<FzDate>(*back).GetEpochDate()));
  Hex("bin", GetChild<FzBin>(*back).GetBuffer(), static_cast<std::size_t>(GetChild<FzBin>(*back).GetSize()));
  auto &bsub = GetChild<FzSub>(*back);
  std::printf("KAT subuint=%llu\n", static_cast<unsigned long long>(static_cast<std::uint64_t>(GetChild<FzSubUInt>(bsub))));
  std::printf("KAT deepstr=%s\n", static_cast<const std::string &>(GetChild<FzDeepStr>(GetChild<FzDeep>(bsub))).c_str());
  delete back;

  // MemIOCallback write / seek / read
  {
    MemIOCallback m(1);
    const char text[] = "0123456789";
    m.write(text, 10);
    m.setFilePointer(3);
    char out[5] = {};
    const auto n = m.read(out, 4);
    std::printf("KAT memio=%zu,%s,%llu,%llu\n", n, out, static_cast<unsigned long long>(m.getFilePointer()),
                static_cast<unsigned long long>(m.GetDataBufferSize()));
    m.setFilePointer(0, seek_end);
    std::printf("KAT memio_atend=%zu\n", m.read(out, 4));
  }

  // EBML header with checksum
  {
    MemIOCallback m;
    EbmlHead head;
    head.EnableChecksum();
    GetChild<EDocTypeVersion>(head).SetValue(1);
    GetChild<EMaxSizeLength>(head).SetValue(7);
    std::printf("KAT head=%llu\n", static_cast<unsigned long long>(head.Render(m, EbmlElement::WriteAll)));
    EbmlStream s(m);
    auto *back2 = Reparse<EbmlHead>(m, s, EBML_INFO(EbmlHead));
    std::printf("KAT headcrc=%d\n", back2 != nullptr && back2->VerifyChecksum());
    if (back2 != nullptr)
      std::printf("KAT doctype=%s\n", static_cast<const std::string &>(GetChild<EDocType>(*back2)).c_str());
    delete back2;
  }
  return 0;
}
