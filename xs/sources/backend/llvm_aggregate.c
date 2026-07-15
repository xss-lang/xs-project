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
  if(type.kind != XS_LIL_TYPE_AGGREGATE && type.kind != XS_LIL_TYPE_ARRAY)
    return xs_llvm_lil_type(unit->backend, type, llvm_type, error);
  LLVMTypeRef *registry = type.kind == XS_LIL_TYPE_ARRAY ? unit->lil_array_types : unit->lil_types;
  size_t count = type.kind == XS_LIL_TYPE_ARRAY ? unit->lil_array_type_count : unit->lil_type_count;
  if(type.registry_id >= count || registry[type.registry_id] == nullptr)
    return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "XLIL composite type is not registered in the codegen unit");
  *llvm_type = registry[type.registry_id];
  return XS_BACKEND_OK;
}

XsBackendStatus xs_llvm_register_lil_types(XsLlvmCodegenUnit *unit, const XsLilModule *module, XsBackendError *error)
{
  if(error != nullptr)
    *error = (XsBackendError){.status = XS_BACKEND_OK};
  if(unit == nullptr || module == nullptr || unit->lil_types != nullptr || unit->lil_array_types != nullptr)
    return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "fresh codegen unit and XLIL module are required");
  size_t count = xs_lil_module_aggregate_type_count(module);
  if(count != 0)
    unit->lil_types = calloc(count, sizeof(*unit->lil_types));
  if(count != 0 && unit->lil_types == nullptr)
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
  size_t array_count = xs_lil_module_array_type_count(module);
  if(array_count != 0)
    unit->lil_array_types = calloc(array_count, sizeof(*unit->lil_array_types));
  if(array_count != 0 && unit->lil_array_types == nullptr)
    return aggregate_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while registering XLIL array types");
  unit->lil_array_type_count = array_count;
  for(size_t index = 0; index < array_count; ++index)
  {
    LLVMTypeRef element = nullptr;
    XsBackendStatus status =
        xs_llvm_codegen_lil_type(unit, xs_lil_module_array_element_type(module, (uint32_t)index), &element, error);
    if(status != XS_BACKEND_OK)
      return status;
    unit->lil_array_types[index] = LLVMArrayType2(element, xs_lil_module_array_length(module, (uint32_t)index));
    if(unit->lil_array_types[index] == nullptr)
      return aggregate_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not create an XLIL array type");
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
  if(kind == XS_LIL_INSTRUCTION_ARRAY_GET || kind == XS_LIL_INSTRUCTION_ARRAY_SET)
  {
    XsLilValueId array = xs_lil_block_instruction_left(block, index);
    XsLilValueId dynamic_index = xs_lil_block_instruction_right(block, index);
    if(result >= value_count || array >= value_count || dynamic_index >= value_count || values[array] == nullptr ||
       values[dynamic_index] == nullptr)
      return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "XLIL dynamic array operand is unavailable");
    LLVMTypeRef array_type = nullptr;
    XsBackendStatus status =
        xs_llvm_codegen_lil_type(unit, xs_lil_function_value_type(function, array), &array_type, error);
    if(status != XS_BACKEND_OK)
      return status;
    LLVMValueRef storage = LLVMBuildAlloca(builder, array_type, "array.dynamic.storage");
    LLVMBuildStore(builder, values[array], storage);
    LLVMTypeRef i64 = LLVMInt64TypeInContext(unit->backend->context);
    LLVMValueRef zero = LLVMConstInt(i64, 0, false);
    LLVMValueRef length = LLVMConstInt(i64, LLVMGetArrayLength2(array_type), false);
    LLVMValueRef nonnegative = LLVMBuildICmp(builder, LLVMIntSGE, values[dynamic_index], zero, "index.nonnegative");
    LLVMValueRef in_range = LLVMBuildICmp(builder, LLVMIntULT, values[dynamic_index], length, "index.in.range");
    LLVMValueRef valid = LLVMBuildAnd(builder, nonnegative, in_range, "index.valid");
    LLVMValueRef llvm_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
    LLVMBasicBlockRef access_block = LLVMAppendBasicBlockInContext(unit->backend->context, llvm_function, "array.access");
    LLVMBasicBlockRef trap_block = LLVMAppendBasicBlockInContext(unit->backend->context, llvm_function, "array.bounds");
    LLVMBuildCondBr(builder, valid, access_block, trap_block);
    LLVMPositionBuilderAtEnd(builder, trap_block);
    LLVMTypeRef trap_type = LLVMFunctionType(LLVMVoidTypeInContext(unit->backend->context), nullptr, 0, false);
    LLVMValueRef trap = LLVMGetNamedFunction(unit->module, "llvm.trap");
    if(trap == nullptr)
      trap = LLVMAddFunction(unit->module, "llvm.trap", trap_type);
    LLVMBuildCall2(builder, trap_type, trap, nullptr, 0, "");
    LLVMBuildUnreachable(builder);
    LLVMPositionBuilderAtEnd(builder, access_block);
    LLVMValueRef indices[] = {zero, values[dynamic_index]};
    LLVMValueRef element = LLVMBuildGEP2(builder, array_type, storage, indices, 2, "array.element");
    if(kind == XS_LIL_INSTRUCTION_ARRAY_GET)
    {
      LLVMTypeRef element_type = nullptr;
      status = xs_llvm_codegen_lil_type(unit, xs_lil_function_value_type(function, result), &element_type, error);
      if(status != XS_BACKEND_OK)
        return status;
      values[result] = LLVMBuildLoad2(builder, element_type, element, "array.get");
    }
    else
    {
      XsLilValueId replacement = xs_lil_block_instruction_argument(block, index, 0);
      if(replacement >= value_count || values[replacement] == nullptr)
        return aggregate_error(error, XS_BACKEND_INVALID_ARGUMENT, "XLIL array.set value is unavailable");
      LLVMBuildStore(builder, values[replacement], element);
      values[result] = LLVMBuildLoad2(builder, array_type, storage, "array.set");
    }
    return values[result] == nullptr
               ? aggregate_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not lower dynamic XLIL array access")
               : XS_BACKEND_OK;
  }
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
