// This file is part of AsmJit project <https://asmjit.com>
//
// See asmjit.h or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#include "../core/api-build_p.h"
#if !defined(ASMJIT_NO_ARM)

#include "../core/cpuinfo.h"
#include "../core/misc_p.h"
#include "../core/support.h"
#include "../arm/a64instapi_p.h"
#include "../arm/a64instdb_p.h"
#include "../arm/a64operand.h"

ASMJIT_BEGIN_SUB_NAMESPACE(a64)

// a64::InstInternal - Text
// ========================

#ifndef ASMJIT_NO_TEXT
Error InstInternal::instIdToString(Arch arch, InstId instId, String& output) noexcept {
  DebugUtils::unused(arch);

  if (ASMJIT_UNLIKELY(!Inst::isDefinedId(instId)))
    return DebugUtils::errored(kErrorInvalidInstruction);

  const InstDB::InstInfo& info = InstDB::infoById(instId);
  return output.append(InstDB::_nameData + info._nameDataIndex);
}

InstId InstInternal::stringToInstId(Arch arch, const char* s, size_t len) noexcept {
  DebugUtils::unused(arch);

  if (ASMJIT_UNLIKELY(!s))
    return Inst::kIdNone;

  if (len == SIZE_MAX)
    len = strlen(s);

  if (ASMJIT_UNLIKELY(len == 0 || len > InstDB::kMaxNameSize))
    return Inst::kIdNone;

  uint32_t prefix = uint32_t(s[0]) - 'a';
  if (ASMJIT_UNLIKELY(prefix > 'z' - 'a'))
    return Inst::kIdNone;

  uint32_t index = InstDB::instNameIndex[prefix].start;
  if (ASMJIT_UNLIKELY(!index))
    return Inst::kIdNone;

  const char* nameData = InstDB::_nameData;
  const InstDB::InstInfo* table = InstDB::_instInfoTable;

  const InstDB::InstInfo* base = table + index;
  const InstDB::InstInfo* end  = table + InstDB::instNameIndex[prefix].end;

  for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
    const InstDB::InstInfo* cur = base + (lim >> 1);
    int result = Support::cmpInstName(nameData + cur[0]._nameDataIndex, s, len);

    if (result < 0) {
      base = cur + 1;
      lim--;
      continue;
    }

    if (result > 0)
      continue;

    return uint32_t((size_t)(cur - table));
  }

  return Inst::kIdNone;
}
#endif // !ASMJIT_NO_TEXT

// a64::InstInternal - Validate
// ============================

#ifndef ASMJIT_NO_VALIDATION
ASMJIT_FAVOR_SIZE Error InstInternal::validate(Arch arch, const BaseInst& inst, const Operand_* operands, size_t opCount, ValidationFlags validationFlags) noexcept {
  // TODO:
  DebugUtils::unused(arch, inst, operands, opCount, validationFlags);
  return kErrorOk;
}
#endif // !ASMJIT_NO_VALIDATION

// a64::InstInternal - QueryRWInfo
// ===============================

#ifndef ASMJIT_NO_INTROSPECTION
struct InstRWInfoData {
  uint8_t rwx[Globals::kMaxOpCount];
};

static const InstRWInfoData instRWInfoData[] = {
  #define R uint8_t(OpRWFlags::kRead)
  #define W uint8_t(OpRWFlags::kWrite)
  #define X uint8_t(OpRWFlags::kRW)

  {{ R, R, R, R, R, R }}, // kRWI_R
  {{ R, W, R, R, R, R }}, // kRWI_RW
  {{ R, X, R, R, R, R }}, // kRWI_RX
  {{ R, R, W, R, R, R }}, // kRWI_RRW
  {{ R, W, X, R, R, R }}, // kRWI_RWX
  {{ W, R, R, R, R, R }}, // kRWI_W
  {{ W, R, W, R, R, R }}, // kRWI_WRW
  {{ W, R, X, R, R, R }}, // kRWI_WRX
  {{ W, R, R, W, R, R }}, // kRWI_WRRW
  {{ W, R, R, X, R, R }}, // kRWI_WRRX
  {{ W, W, R, R, R, R }}, // kRWI_WW
  {{ X, R, R, R, R, R }}, // kRWI_X
  {{ X, R, X, R, R, R }}, // kRWI_XRX
  {{ X, X, R, R, X, R }}, // kRWI_XXRRX

  {{ W, R, R, R, R, R }}, // kRWI_LDn
  {{ R, W, R, R, R, R }}, // kRWI_STn
  {{ R, R, R, R, R, R }}  // kRWI_TODO

  #undef R
  #undef W
  #undef X
};

static const uint8_t elementTypeSize[8] = { 0, 1, 2, 4, 8, 4, 4, 0 };

Error InstInternal::queryRWInfo(Arch arch, const BaseInst& inst, const Operand_* operands, size_t opCount, InstRWInfo* out) noexcept {
  // Unused in Release configuration as the assert is not compiled in.
  DebugUtils::unused(arch);

  // Only called when `arch` matches X86 family.
  ASMJIT_ASSERT(Environment::isFamilyARM(arch));

  // Get the instruction data.
  InstId instId = inst.id();
  if (ASMJIT_UNLIKELY(!Inst::isDefinedId(instId)))
    return DebugUtils::errored(kErrorInvalidInstruction);

  out->_instFlags = 0;
  out->_opCount = uint8_t(opCount);
  out->_rmFeature = 0;
  out->_extraReg.reset();
  out->_readFlags = CpuRWFlags::kNone; // TODO: [ARM] Read PSTATUS.
  out->_writeFlags = CpuRWFlags::kNone; // TODO: [ARM] Write PSTATUS

  const InstDB::InstInfo& instInfo = InstDB::_instInfoTable[instId];
  const InstRWInfoData& rwInfo = instRWInfoData[instInfo.rwInfoIndex()];

  if (instInfo.hasFlag(InstDB::kInstFlagConsecutive) && opCount > 2) {
    for (uint32_t i = 0; i < opCount; i++) {
      OpRWInfo& op = out->_operands[i];
      const Operand_& srcOp = operands[i];

      if (!srcOp.isRegOrMem()) {
        op.reset();
        continue;
      }

      OpRWFlags rwFlags = i < opCount -1 ? (OpRWFlags)rwInfo.rwx[0] : (OpRWFlags)rwInfo.rwx[1];

      op._opFlags = rwFlags & ~(OpRWFlags::kZExt);
      op._physId = BaseReg::kIdBad;
      op._rmSize = 0;
      op._resetReserved();

      uint64_t rByteMask = op.isRead() ? 0xFFFFFFFFFFFFFFFFu : 0x0000000000000000u;
      uint64_t wByteMask = op.isWrite() ? 0xFFFFFFFFFFFFFFFFu : 0x0000000000000000u;

      op._readByteMask = rByteMask;
      op._writeByteMask = wByteMask;
      op._extendByteMask = 0;

      if (srcOp.isReg()) {
        if (i == 0)
          op._consecutiveLeadCount = uint8_t(opCount - 1);
        else
          op.addOpFlags(OpRWFlags::kConsecutive);
      }
      else {
        const Mem& memOp = srcOp.as<Mem>();

        if (memOp.hasBase()) {
          op.addOpFlags(OpRWFlags::kMemBaseRead);
        }

        if (memOp.hasIndex()) {
          op.addOpFlags(OpRWFlags::kMemIndexRead);
          op.addOpFlags(memOp.isPreOrPost() ? OpRWFlags::kMemIndexWrite : OpRWFlags::kNone);
        }
      }
    }
  }
  else {
    for (uint32_t i = 0; i < opCount; i++) {
      OpRWInfo& op = out->_operands[i];
      const Operand_& srcOp = operands[i];

      if (!srcOp.isRegOrMem()) {
        op.reset();
        continue;
      }

      OpRWFlags rwFlags = (OpRWFlags)rwInfo.rwx[i];

      op._opFlags = rwFlags & ~(OpRWFlags::kZExt);
      op._physId = BaseReg::kIdBad;
      op._rmSize = 0;
      op._resetReserved();

      uint64_t rByteMask = op.isRead() ? 0xFFFFFFFFFFFFFFFFu : 0x0000000000000000u;
      uint64_t wByteMask = op.isWrite() ? 0xFFFFFFFFFFFFFFFFu : 0x0000000000000000u;

      op._readByteMask = rByteMask;
      op._writeByteMask = wByteMask;
      op._extendByteMask = 0;

      if (srcOp.isReg()) {
        if (srcOp.as<Vec>().hasElementIndex()) {
          // Only part of the vector is accessed if element index [] is used.
          uint32_t elementType = srcOp.as<Vec>().elementType();
          uint32_t elementIndex = srcOp.as<Vec>().elementIndex();

          uint32_t elementSize = elementTypeSize[elementType];
          uint64_t accessMask = uint64_t(Support::lsbMask<uint32_t>(elementSize)) << (elementIndex * elementSize);

          op._readByteMask &= accessMask;
          op._writeByteMask &= accessMask;
        }

        // TODO: [ARM] RW info is not finished.
      }
      else {
        const Mem& memOp = srcOp.as<Mem>();

        if (memOp.hasBase()) {
          op.addOpFlags(OpRWFlags::kMemBaseRead);
        }

        if (memOp.hasIndex()) {
          op.addOpFlags(OpRWFlags::kMemIndexRead);
          op.addOpFlags(memOp.isPreOrPost() ? OpRWFlags::kMemIndexWrite : OpRWFlags::kNone);
        }
      }
    }
  }

  return kErrorOk;
}
#endif // !ASMJIT_NO_INTROSPECTION

// a64::InstInternal - QueryFeatures
// =================================

#ifndef ASMJIT_NO_INTROSPECTION
Error InstInternal::queryFeatures(Arch arch, const BaseInst& inst, const Operand_* operands, size_t opCount, CpuFeatures* out) noexcept {
  // TODO: [ARM] QueryFeatures not implemented yet.
  DebugUtils::unused(arch, inst, operands, opCount, out);
  return kErrorOk;
}
#endif // !ASMJIT_NO_INTROSPECTION

// a64::InstInternal - Unit
// ========================

#if defined(ASMJIT_TEST)
UNIT(arm_inst_api_text) {
  // TODO:
}
#endif

ASMJIT_END_SUB_NAMESPACE

#endif // !ASMJIT_NO_ARM
