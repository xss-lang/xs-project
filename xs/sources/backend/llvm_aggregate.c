/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "llvm_internal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static XsBackendStatus aggregate_error(XsBackendError *error, XsBackendStatus status, const char *message)
{
  if(error != nullptr)
  {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message);
  }
  return status;
}

XsBackendStatus xs_llvm_codegen_lil_type(XsLlvmCodegenUnit *unit, XsLilType type, LLVMTypeRef *llvm_type,
                                         XsBackendError *error)
{
  if(type.kind != XS_LIL_TYPE_AGGREGATE)
    return xs_llvm_lil_type(unit->backend, type, llvm_type, error);
  if(type.registry_id >= unit->lil_type_count || unit->lil_types[type.registry_id] == nullptr)
    return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT,
                           "XLIL aggregate type is not registered in the codegen unit");
  *llvm_type = unit->lil_types[type.registry_id];
  return XS_BACKEND_OK;
}

XsBackendStatus xs_llvm_register_lil_types(XsLlvmCodegenUnit *unit, const XsLilModule *module, XsBackendError *error)
{
  if(error != nullptr)
    *error = (XsBackendError){.status = XS_BACKEND_OK};
  if(unit == nullptr || module == nullptr || unit->lil_types != nullptr)
    return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "fresh codegen unit and XLIL module are required");
  size_t count = xs_lil_module_aggregate_type_count(module);
  if(count == 0)
    return XS_BACKEND_OK;
  unit->lil_types = calloc(count, sizeof(*unit->lil_types));
  if(unit->lil_types == nullptr)
    return aggregate_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while registering XLIL aggregate types");
  unit->lil_type_count = count;
  for(size_t index = 0; index < count; ++index)
  {
    unit->lil_types[index] =
        LLVMStructCreateNamed(unit->backend->context, xs_lil_module_aggregate_type_name(module, (uint32_t)index));
    if(unit->lil_types[index] == nullptr)
      return aggregate_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not create an XLIL aggregate type");
  }
  for(size_t index = 0; index < count; ++index)
  {
    size_t field_count = xs_lil_module_aggregate_field_count(module, (uint32_t)index);
    LLVMTypeRef *fields = field_count == 0 ? nullptr : malloc(field_count * sizeof(*fields));
    if(field_count != 0 && fields == nullptr)
      return aggregate_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while lowering XLIL aggregate fields");
    XsBackendStatus status = XS_BACKEND_OK;
    for(size_t field = 0; field < field_count && status == XS_BACKEND_OK; ++field)
      status = xs_llvm_codegen_lil_type(unit, xs_lil_module_aggregate_field_type(module, (uint32_t)index, field),
                                        &fields[field], error);
    if(status == XS_BACKEND_OK)
      LLVMStructSetBody(unit->lil_types[index], fields, (unsigned)field_count, false);
    free(fields);
    if(status != XS_BACKEND_OK)
      return status;
  }
  return XS_BACKEND_OK;
}

XsBackendStatus xs_llvm_lower_aggregate_instruction(XsLlvmCodegenUnit *unit, LLVMBuilderRef builder,
                                                    const XsLilFunction *function, const XsLilBlock *block,
                                                    size_t index, LLVMValueRef *values, size_t value_count,
                                                    XsBackendError *error)
{
  XsLilInstructionKind kind = xs_lil_block_instruction_kind(block, index);
  XsLilValueId result = xs_lil_block_instruction_result(block, index);
  if(kind == XS_LIL_INSTRUCTION_EXTRACT)
  {
    XsLilValueId aggregate = xs_lil_block_instruction_left(block, index);
    int64_t field = xs_lil_block_instruction_i64(block, index);
    if(aggregate >= value_count || values[aggregate] == nullptr || field < 0 || (uint64_t)field > UINT_MAX)
      return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "XLIL extract source or field is unavailable");
    values[result] = LLVMBuildExtractValue(builder, values[aggregate], (unsigned)field, "extract");
    return values[result] == nullptr
               ? aggregate_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not extract an XLIL aggregate field")
               : XS_BACKEND_OK;
  }
  LLVMTypeRef type = nullptr;
  XsBackendStatus status = xs_llvm_codegen_lil_type(unit, xs_lil_function_value_type(function, result), &type, error);
  if(status != XS_BACKEND_OK)
    return status;
  LLVMValueRef aggregate = LLVMGetUndef(type);
  size_t field_count = xs_lil_block_instruction_argument_count(block, index);
  for(size_t field = 0; field < field_count; ++field)
  {
    XsLilValueId value = xs_lil_block_instruction_argument(block, index, field);
    if(value >= value_count || values[value] == nullptr)
      return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "XLIL aggregate references an unavailable field");
    aggregate = LLVMBuildInsertValue(builder, aggregate, values[value], (unsigned)field, "aggregate");
  }
  values[result] = aggregate;
  return XS_BACKEND_OK;
}
