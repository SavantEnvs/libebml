// Fuzzing schema: one element of every libebml value type, two levels of nested masters.
#ifndef FZ_SCHEMA_H
#define FZ_SCHEMA_H

#include <ebml/EbmlBinary.h>
#include <ebml/EbmlContexts.h>
#include <ebml/EbmlCrc32.h>
#include <ebml/EbmlDate.h>
#include <ebml/EbmlFloat.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlMaster.h>
#include <ebml/EbmlSInteger.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlString.h>
#include <ebml/EbmlUInteger.h>
#include <ebml/EbmlUnicodeString.h>
#include <ebml/EbmlVoid.h>
#include <ebml/MemIOCallback.h>
#include <ebml/MemReadIOCallback.h>

using namespace libebml;

static constexpr EbmlDocVersion FzVersions{"fuzz"};

DECLARE_xxx_UINTEGER(FzUInt,)       EBML_CONCRETE_CLASS(FzUInt) };
DECLARE_xxx_SINTEGER(FzSInt,)       EBML_CONCRETE_CLASS(FzSInt) };
DECLARE_xxx_STRING(FzStr,)          EBML_CONCRETE_CLASS(FzStr) };
DECLARE_xxx_UNISTRING(FzUni,)       EBML_CONCRETE_CLASS(FzUni) };
DECLARE_xxx_FLOAT(FzFloat,)         EBML_CONCRETE_CLASS(FzFloat) };
DECLARE_xxx_DATE(FzDate,)           EBML_CONCRETE_CLASS(FzDate) };
DECLARE_xxx_BINARY(FzBin,)          EBML_CONCRETE_CLASS(FzBin) };
DECLARE_xxx_UINTEGER(FzSubUInt,)    EBML_CONCRETE_CLASS(FzSubUInt) };
DECLARE_xxx_BINARY(FzSubBin,)       EBML_CONCRETE_CLASS(FzSubBin) };
DECLARE_xxx_STRING(FzDeepStr,)      EBML_CONCRETE_CLASS(FzDeepStr) };
DECLARE_xxx_UNISTRING(FzDeepUni,)   EBML_CONCRETE_CLASS(FzDeepUni) };

DEFINE_START_SEMANTIC(FzDeep)
DEFINE_SEMANTIC_ITEM(false, false, FzDeepStr)
DEFINE_SEMANTIC_ITEM(false, false, FzDeepUni)
DEFINE_END_SEMANTIC(FzDeep)
DECLARE_xxx_MASTER(FzDeep,)         EBML_CONCRETE_CLASS(FzDeep) };

DEFINE_START_SEMANTIC(FzSub)
DEFINE_SEMANTIC_ITEM(false, false, FzSubUInt)
DEFINE_SEMANTIC_ITEM(false, false, FzSubBin)
DEFINE_SEMANTIC_ITEM(false, false, FzDeep)
DEFINE_END_SEMANTIC(FzSub)
DECLARE_xxx_MASTER(FzSub,)          EBML_CONCRETE_CLASS(FzSub) };

DEFINE_START_SEMANTIC(FzRoot)
DEFINE_SEMANTIC_ITEM(false, false, FzUInt)
DEFINE_SEMANTIC_ITEM(false, false, FzSInt)
DEFINE_SEMANTIC_ITEM(false, false, FzStr)
DEFINE_SEMANTIC_ITEM(false, false, FzUni)
DEFINE_SEMANTIC_ITEM(false, false, FzFloat)
DEFINE_SEMANTIC_ITEM(false, false, FzDate)
DEFINE_SEMANTIC_ITEM(false, false, FzBin)
DEFINE_SEMANTIC_ITEM(false, false, FzSub)
DEFINE_END_SEMANTIC(FzRoot)
DECLARE_xxx_MASTER(FzRoot,)         EBML_CONCRETE_CLASS(FzRoot) };

DEFINE_EBML_MASTER_ORPHAN(FzRoot, 0x1B4D5A01, true, "FzRoot", FzVersions)
FzRoot::FzRoot() : EbmlMaster(FzRoot::ClassInfos) {}

DEFINE_xxx_UINTEGER(FzUInt,  0x81,   FzRoot, "FzUInt",  FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_SINTEGER(FzSInt,  0x82,   FzRoot, "FzSInt",  FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_STRING  (FzStr,   0x83,   FzRoot, "FzStr",   FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_UNISTRING(FzUni,  0x84,   FzRoot, "FzUni",   FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_FLOAT   (FzFloat, 0x85,   FzRoot, "FzFloat", FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_DATE    (FzDate,  0x86,   FzRoot, "FzDate",  FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_BINARY  (FzBin,   0x4687, FzRoot, "FzBin",   FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_MASTER  (FzSub,   0xA0,   FzRoot, true, "FzSub", FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_UINTEGER(FzSubUInt, 0x91, FzSub,  "FzSubUInt", FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_BINARY  (FzSubBin,  0x4792, FzSub, "FzSubBin", FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_MASTER  (FzDeep,  0xA3,   FzSub,  false, "FzDeep", FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_STRING  (FzDeepStr, 0x93, FzDeep, "FzDeepStr", FzVersions, GetEbmlGlobal_Context)
DEFINE_xxx_UNISTRING(FzDeepUni, 0x94, FzDeep, "FzDeepUni", FzVersions, GetEbmlGlobal_Context)

#endif
