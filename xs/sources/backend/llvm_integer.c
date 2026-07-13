/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "llvm_integer.h"

#include <stdio.h>

static bool integer_constant_type(XsLilInstructionKind kind, XsLilTypeKind *type)
{
  switch(kind)
  {
  case XS_LIL_INSTRUCTION_CONST_U8:
    *type = XS_LIL_TYPE_U8;
    return true;
  case XS_LIL_INSTRUCTION_CONST_I8:
    *type = XS_LIL_TYPE_I8;
    return true;
  case XS_LIL_INSTRUCTION_CONST_U16:
    *type = XS_LIL_TYPE_U16;
    return true;
  case XS_LIL_INSTRUCTION_CONST_I16:
    *type = XS_LIL_TYPE_I16;
    return true;
  case XS_LIL_INSTRUCTION_CONST_U32:
    *type = XS_LIL_TYPE_U32;
    return true;
  case XS_LIL_INSTRUCTION_CONST_U64:
    *type = XS_LIL_TYPE_U64;
    return true;
  case XS_LIL_INSTRUCTION_CONST_U128:
    *type = XS_LIL_TYPE_U128;
    return true;
  case XS_LIL_INSTRUCTION_CONST_I128:
    *type = XS_LIL_TYPE_I128;
    return true;
  default:
    return false;
  }
}

bool xs_llvm_is_integer_constant(XsLilInstructionKind kind)
{
  XsLilTypeKind unused = XS_LIL_TYPE_VOID;
  return integer_constant_type(kind, &unused);
}

XsBackendStatus xs_llvm_lower_integer_constant(XsLlvmBackend *backend, const XsLilBlock *block, size_t index,
                                               LLVMValueRef *value, XsBackendError *error)
{
  XsLilTypeKind integer_kind = XS_LIL_TYPE_VOID;
  if(value == nullptr || !integer_constant_type(xs_lil_block_instruction_kind(block, index), &integer_kind))
  {
    if(error != nullptr)
    {
      error->status = XS_BACKEND_INVALID_ARGUMENT;
      snprintf(error->message, sizeof(error->message), "%s", "valid XLIL integer constant is required");
    }
    return XS_BACKEND_INVALID_ARGUMENT;
  }
  LLVMTypeRef type = nullptr;
  XsBackendStatus status = xs_llvm_lil_type(backend, (XsLilType){.kind = integer_kind}, &type, error);
  if(status != XS_BACKEND_OK)
    return status;
  XsUInt128 bits = xs_lil_block_instruction_integer_bits(block, index);
  if(integer_kind == XS_LIL_TYPE_U128 || integer_kind == XS_LIL_TYPE_I128)
  {
    uint64_t words[2] = {bits.low, bits.high};
    *value = LLVMConstIntOfArbitraryPrecision(type, 2, words);
  }
  else
  {
    *value = LLVMConstInt(type, bits.low, false);
  }
  return XS_BACKEND_OK;
}
