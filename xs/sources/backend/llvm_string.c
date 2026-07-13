/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "llvm_string.h"

#include <llvm-c/Core.h>

#include <stdio.h>
#include <stdlib.h>

static XsBackendStatus fail(XsBackendError *error, XsBackendStatus status, const char *message)
{
  if(error != nullptr)
  {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message);
  }
  return status;
}

XsBackendStatus xs_llvm_lower_lil_const_str(XsLlvmBackend *backend, XsLlvmCodegenUnit *unit, const XsLilBlock *block,
                                            size_t index, LLVMValueRef *value, XsBackendError *error)
{
  size_t unit_count = xs_lil_block_instruction_utf16_length(block, index);
  if(unit_count > SIZE_MAX / 2U)
    return fail(error, XS_BACKEND_INVALID_ARGUMENT, "XLIL const.str is too large for LLVM lowering");
  size_t byte_count = unit_count * 2U;
  LLVMContextRef context = LLVMGetModuleContext(xs_llvm_codegen_unit_module(unit));
  LLVMTypeRef byte_type = LLVMInt8TypeInContext(context);
  LLVMValueRef *bytes = calloc(byte_count == 0 ? 1 : byte_count, sizeof(*bytes));
  if(bytes == nullptr)
    return fail(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while lowering XLIL const.str");
  XsLilUtf16Encoding encoding = xs_lil_block_instruction_utf16_encoding(block, index);
  for(size_t current = 0; current < unit_count; ++current)
  {
    uint16_t code_unit = xs_lil_block_instruction_utf16_unit(block, index, current);
    unsigned low = code_unit & 0xFFU;
    unsigned high = code_unit >> 8U;
    bytes[current * 2U] = LLVMConstInt(byte_type, encoding == XS_LIL_UTF16_LE ? low : high, false);
    bytes[current * 2U + 1U] = LLVMConstInt(byte_type, encoding == XS_LIL_UTF16_LE ? high : low, false);
  }
  LLVMTypeRef array_type = LLVMArrayType2(byte_type, (uint64_t)byte_count);
  LLVMValueRef initializer = LLVMConstArray2(byte_type, bytes, (uint64_t)byte_count);
  free(bytes);
  LLVMValueRef global = LLVMAddGlobal(xs_llvm_codegen_unit_module(unit), array_type, ".xs.str");
  LLVMSetInitializer(global, initializer);
  LLVMSetGlobalConstant(global, true);
  LLVMSetLinkage(global, LLVMPrivateLinkage);
  LLVMSetUnnamedAddress(global, LLVMGlobalUnnamedAddr);
  LLVMSetAlignment(global, 2);

  LLVMTypeRef str_type = nullptr;
  XsBackendStatus status = xs_llvm_lil_type(backend, (XsLilType){.kind = XS_LIL_TYPE_STR}, &str_type, error);
  if(status != XS_BACKEND_OK)
    return status;
  LLVMTypeRef length_type = LLVMStructGetTypeAtIndex(str_type, 1);
  LLVMValueRef fields[] = {global, LLVMConstInt(length_type, (unsigned long long)unit_count, false)};
  *value = LLVMConstNamedStruct(str_type, fields, 2);
  return *value == nullptr ? fail(error, XS_BACKEND_LLVM_ERROR, "LLVM could not create XLIL str view") : XS_BACKEND_OK;
}
