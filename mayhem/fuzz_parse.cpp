// Parse an EBML stream (EBML head and/or a FzRoot tree) through EbmlStream / EbmlMaster::Read,
// then walk the tree, re-render it and re-parse the rendering.
#include "fz_schema.h"

#include <cstdint>
#include <cstdio>
#include <exception>

static std::uint64_t g_sink;

static void Walk(const EbmlElement *el, int depth)
{
  if (el == nullptr || depth > 64)
    return;
  g_sink += EBML_ID_VALUE(static_cast<const EbmlId &>(*el)) + el->GetSize() + el->GetElementPosition();
  if (el->IsMaster()) {
    const auto *m = static_cast<const EbmlMaster *>(el);
    g_sink += m->VerifyChecksum();
    for (const EbmlElement *child : m->GetElementList())
      Walk(child, depth + 1);
  } else if (auto *u = dynamic_cast<const EbmlUInteger *>(el)) {
    g_sink += static_cast<std::uint64_t>(*u);
  } else if (auto *s = dynamic_cast<const EbmlSInteger *>(el)) {
    g_sink += static_cast<std::uint64_t>(static_cast<std::int64_t>(*s));
  } else if (auto *str = dynamic_cast<const EbmlString *>(el)) {
    g_sink += static_cast<const std::string &>(*str).size();
  } else if (auto *uni = dynamic_cast<const EbmlUnicodeString *>(el)) {
    g_sink += uni->GetValueUTF8().size();
  } else if (auto *f = dynamic_cast<const EbmlFloat *>(el)) {
    g_sink += static_cast<std::uint64_t>(static_cast<double>(*f) > 0.0);
  } else if (auto *d = dynamic_cast<const EbmlDate *>(el)) {
    g_sink += static_cast<std::uint64_t>(d->GetEpochDate());
  } else if (auto *b = dynamic_cast<const EbmlBinary *>(el)) {
    if (b->GetBuffer() != nullptr && b->GetSize() != 0)
      g_sink += b->GetBuffer()[0] + b->GetBuffer()[b->GetSize() - 1];
  }
}

static EbmlMaster *ParseTop(EbmlStream &stream, IOCallback &io, std::uint64_t size)
{
  const std::uint64_t start = io.getFilePointer();
  EbmlElement *el = stream.FindNextID(EBML_INFO(FzRoot), size);
  if (el == nullptr || !(EbmlId(*el) == EBML_ID(FzRoot))) {
    delete el;
    io.setFilePointer(static_cast<std::int64_t>(start), seek_beginning);
    el = stream.FindNextID(EBML_INFO(EbmlHead), size);
    if (el != nullptr && !(EbmlId(*el) == EBML_ID(EbmlHead))) {
      delete el;
      el = nullptr;
    }
  }
  if (el == nullptr)
    return nullptr;

  auto *master = static_cast<EbmlMaster *>(el);
  int upper = 0;
  EbmlElement *found = nullptr;
  try {
    master->Read(stream, EBML_CONTEXT(master), upper, found, true);
  } catch (const std::exception &) {
    // truncated data: readFully() reports EOF by throwing, as callers are expected to handle
  }
  if (found != nullptr && found != master) {
    bool child = false;
    for (const EbmlElement *c : master->GetElementList())
      child |= (c == found);
    if (!child)
      delete found;
  }
  return master;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
  MemReadIOCallback io(data, size);
  EbmlStream stream(io);

  EbmlMaster *master = ParseTop(stream, io, size);
  if (master == nullptr)
    return -1;

  Walk(master, 0);
  g_sink += master->CheckMandatory();

  MemIOCallback out;
  master->UpdateSize(EbmlElement::WriteAll, true);
  master->Render(out, EbmlElement::WriteAll);

  if (out.GetDataBufferSize() != 0) {
    MemReadIOCallback io2(out.GetDataBuffer(), out.GetDataBufferSize());
    EbmlStream stream2(io2);
    EbmlMaster *again = ParseTop(stream2, io2, out.GetDataBufferSize());
    Walk(again, 0);
    delete again;
  }
  delete master;
  return 0;
}
