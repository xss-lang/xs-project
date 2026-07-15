/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_LIL_H
#define XS_LIL_H

#include "xs/int128.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum
{
  XS_LIL_OK,
  XS_LIL_INVALID_ARGUMENT,
  XS_LIL_ALLOCATION_FAILED,
  XS_LIL_IO_ERROR,
} XsLilStatus;

typedef struct
{
  XsLilStatus status;
  char message[256];
} XsLilError;

typedef enum
{
  XS_LIL_TYPE_VOID,
  XS_LIL_TYPE_BOOL,
  XS_LIL_TYPE_U8,
  XS_LIL_TYPE_I8,
  XS_LIL_TYPE_U16,
  XS_LIL_TYPE_I16,
  XS_LIL_TYPE_U32,
  XS_LIL_TYPE_I32,
  XS_LIL_TYPE_U64,
  XS_LIL_TYPE_I64,
  XS_LIL_TYPE_U128,
  XS_LIL_TYPE_I128,
  XS_LIL_TYPE_F16,
  XS_LIL_TYPE_F32,
  XS_LIL_TYPE_F64,
  XS_LIL_TYPE_F128,
  XS_LIL_TYPE_STR,
  XS_LIL_TYPE_AGGREGATE,
  XS_LIL_TYPE_ARRAY,
} XsLilTypeKind;

typedef struct
{
  XsLilTypeKind kind;
  uint32_t registry_id;
} XsLilType;

typedef uint32_t XsLilValueId;
typedef uint32_t XsLilBlockId;
typedef uint32_t XsLilSlotId;

typedef enum
{
  XS_LIL_UTF16_LE,
  XS_LIL_UTF16_BE,
} XsLilUtf16Encoding;

typedef enum
{
  XS_LIL_INSTRUCTION_CONST_I64,
  XS_LIL_INSTRUCTION_CONST_I32,
  XS_LIL_INSTRUCTION_CONST_BOOL,
  XS_LIL_INSTRUCTION_ADD_I64,
  XS_LIL_INSTRUCTION_SUB_I64,
  XS_LIL_INSTRUCTION_MUL_I64,
  XS_LIL_INSTRUCTION_DIV_I64,
  XS_LIL_INSTRUCTION_REM_I64,
  XS_LIL_INSTRUCTION_AND_I64,
  XS_LIL_INSTRUCTION_OR_I64,
  XS_LIL_INSTRUCTION_XOR_I64,
  XS_LIL_INSTRUCTION_SHL_I64,
  XS_LIL_INSTRUCTION_SHR_I64,
  XS_LIL_INSTRUCTION_EQ_I64,
  XS_LIL_INSTRUCTION_NE_I64,
  XS_LIL_INSTRUCTION_LT_I64,
  XS_LIL_INSTRUCTION_LE_I64,
  XS_LIL_INSTRUCTION_GT_I64,
  XS_LIL_INSTRUCTION_GE_I64,
  XS_LIL_INSTRUCTION_ADD_I32,
  XS_LIL_INSTRUCTION_SUB_I32,
  XS_LIL_INSTRUCTION_MUL_I32,
  XS_LIL_INSTRUCTION_DIV_I32,
  XS_LIL_INSTRUCTION_REM_I32,
  XS_LIL_INSTRUCTION_AND_I32,
  XS_LIL_INSTRUCTION_OR_I32,
  XS_LIL_INSTRUCTION_XOR_I32,
  XS_LIL_INSTRUCTION_SHL_I32,
  XS_LIL_INSTRUCTION_SHR_I32,
  XS_LIL_INSTRUCTION_EQ_I32,
  XS_LIL_INSTRUCTION_NE_I32,
  XS_LIL_INSTRUCTION_LT_I32,
  XS_LIL_INSTRUCTION_LE_I32,
  XS_LIL_INSTRUCTION_GT_I32,
  XS_LIL_INSTRUCTION_GE_I32,
  XS_LIL_INSTRUCTION_NOT_BOOL,
  XS_LIL_INSTRUCTION_CALL,
  XS_LIL_INSTRUCTION_LOAD,
  XS_LIL_INSTRUCTION_STORE,
  XS_LIL_INSTRUCTION_CONST_F32,
  XS_LIL_INSTRUCTION_CONST_F64,
  XS_LIL_INSTRUCTION_ADD_F32,
  XS_LIL_INSTRUCTION_SUB_F32,
  XS_LIL_INSTRUCTION_MUL_F32,
  XS_LIL_INSTRUCTION_DIV_F32,
  XS_LIL_INSTRUCTION_REM_F32,
  XS_LIL_INSTRUCTION_EQ_F32,
  XS_LIL_INSTRUCTION_NE_F32,
  XS_LIL_INSTRUCTION_LT_F32,
  XS_LIL_INSTRUCTION_LE_F32,
  XS_LIL_INSTRUCTION_GT_F32,
  XS_LIL_INSTRUCTION_GE_F32,
  XS_LIL_INSTRUCTION_ADD_F64,
  XS_LIL_INSTRUCTION_SUB_F64,
  XS_LIL_INSTRUCTION_MUL_F64,
  XS_LIL_INSTRUCTION_DIV_F64,
  XS_LIL_INSTRUCTION_REM_F64,
  XS_LIL_INSTRUCTION_EQ_F64,
  XS_LIL_INSTRUCTION_NE_F64,
  XS_LIL_INSTRUCTION_LT_F64,
  XS_LIL_INSTRUCTION_LE_F64,
  XS_LIL_INSTRUCTION_GT_F64,
  XS_LIL_INSTRUCTION_GE_F64,
  XS_LIL_INSTRUCTION_CONST_STR,
  XS_LIL_INSTRUCTION_EQ_STR,
  XS_LIL_INSTRUCTION_NE_STR,
  XS_LIL_INSTRUCTION_CONST_U16,
  XS_LIL_INSTRUCTION_CONST_U8,
  XS_LIL_INSTRUCTION_CONST_I8,
  XS_LIL_INSTRUCTION_CONST_I16,
  XS_LIL_INSTRUCTION_CONST_U32,
  XS_LIL_INSTRUCTION_CONST_U64,
  XS_LIL_INSTRUCTION_CONST_U128,
  XS_LIL_INSTRUCTION_CONST_I128,
  XS_LIL_INSTRUCTION_BINARY_INTEGER,
  XS_LIL_INSTRUCTION_AGGREGATE,
  XS_LIL_INSTRUCTION_EXTRACT,
  XS_LIL_INSTRUCTION_ARRAY_GET,
  XS_LIL_INSTRUCTION_ARRAY_SET,
} XsLilInstructionKind;

typedef enum
{
  XS_LIL_INTEGER_ADD,
  XS_LIL_INTEGER_SUB,
  XS_LIL_INTEGER_MUL,
  XS_LIL_INTEGER_DIV,
  XS_LIL_INTEGER_REM,
  XS_LIL_INTEGER_BIT_AND,
  XS_LIL_INTEGER_BIT_OR,
  XS_LIL_INTEGER_BIT_XOR,
  XS_LIL_INTEGER_SHIFT_LEFT,
  XS_LIL_INTEGER_SHIFT_RIGHT,
  XS_LIL_INTEGER_EQUAL,
  XS_LIL_INTEGER_NOT_EQUAL,
  XS_LIL_INTEGER_LESS,
  XS_LIL_INTEGER_LESS_EQUAL,
  XS_LIL_INTEGER_GREATER,
  XS_LIL_INTEGER_GREATER_EQUAL,
} XsLilIntegerBinaryOperation;

typedef enum
{
  XS_LIL_FLOAT_ADD,
  XS_LIL_FLOAT_SUB,
  XS_LIL_FLOAT_MUL,
  XS_LIL_FLOAT_DIV,
  XS_LIL_FLOAT_REM,
} XsLilFloatBinaryOperation;

typedef enum
{
  XS_LIL_FLOAT_EQ,
  XS_LIL_FLOAT_NE,
  XS_LIL_FLOAT_LT,
  XS_LIL_FLOAT_LE,
  XS_LIL_FLOAT_GT,
  XS_LIL_FLOAT_GE,
} XsLilFloatComparisonOperation;

typedef enum
{
  XS_LIL_STR_EQ,
  XS_LIL_STR_NE,
} XsLilStrComparisonOperation;

typedef enum
{
  XS_LIL_TERMINATOR_NONE,
  XS_LIL_TERMINATOR_RETURN,
  XS_LIL_TERMINATOR_BRANCH,
  XS_LIL_TERMINATOR_BRANCH_IF,
  XS_LIL_TERMINATOR_PANIC,
} XsLilTerminatorKind;

typedef struct XsLilBlock XsLilBlock;
typedef struct XsLilFunction XsLilFunction;
typedef struct XsLilModule XsLilModule;

XsLilStatus xs_lil_module_create(const char *name, XsLilModule **module, XsLilError *error);
XsLilStatus xs_lil_module_parse_text(const char *path, const char *text, size_t length, XsLilModule **module,
                                     XsLilError *error);
XsLilStatus xs_lil_module_verify(const XsLilModule *module, XsLilError *error);
void xs_lil_module_destroy(XsLilModule *module);
const char *xs_lil_module_name(const XsLilModule *module);
XsLilType xs_lil_aggregate_type(uint32_t registry_id);
XsLilType xs_lil_array_type(uint32_t registry_id);
bool xs_lil_type_equal(XsLilType left, XsLilType right);
XsLilStatus xs_lil_module_add_aggregate_type(XsLilModule *module, const char *name, const XsLilType *fields,
                                             size_t field_count, XsLilType *type, XsLilError *error);
size_t xs_lil_module_aggregate_type_count(const XsLilModule *module);
const char *xs_lil_module_aggregate_type_name(const XsLilModule *module, uint32_t registry_id);
size_t xs_lil_module_aggregate_field_count(const XsLilModule *module, uint32_t registry_id);
XsLilType xs_lil_module_aggregate_field_type(const XsLilModule *module, uint32_t registry_id, size_t field);
XsLilStatus xs_lil_module_add_array_type(XsLilModule *module, XsLilType element_type, uint64_t length,
                                         XsLilType *type, XsLilError *error);
size_t xs_lil_module_array_type_count(const XsLilModule *module);
XsLilType xs_lil_module_array_element_type(const XsLilModule *module, uint32_t registry_id);
uint64_t xs_lil_module_array_length(const XsLilModule *module, uint32_t registry_id);

XsLilStatus xs_lil_module_add_function(XsLilModule *module, const char *name, XsLilType return_type,
                                       const XsLilType *parameters, size_t parameter_count, XsLilError *error);
XsLilStatus xs_lil_module_add_function_definition(XsLilModule *module, const char *name, XsLilType return_type,
                                                  const XsLilType *parameters, size_t parameter_count,
                                                  XsLilFunction **function, XsLilError *error);
size_t xs_lil_module_function_count(const XsLilModule *module);
const XsLilFunction *xs_lil_module_function_at(const XsLilModule *module, size_t index);
const char *xs_lil_function_name(const XsLilFunction *function);
XsLilType xs_lil_function_return_type(const XsLilFunction *function);
size_t xs_lil_function_parameter_count(const XsLilFunction *function);
XsLilType xs_lil_function_parameter_type(const XsLilFunction *function, size_t index);
bool xs_lil_function_is_definition(const XsLilFunction *function);
size_t xs_lil_function_value_count(const XsLilFunction *function);
XsLilType xs_lil_function_value_type(const XsLilFunction *function, XsLilValueId value);
XsLilStatus xs_lil_function_add_slot(XsLilFunction *function, XsLilType type, XsLilSlotId *slot, XsLilError *error);
size_t xs_lil_function_slot_count(const XsLilFunction *function);
XsLilType xs_lil_function_slot_type(const XsLilFunction *function, XsLilSlotId slot);
size_t xs_lil_function_block_count(const XsLilFunction *function);
const XsLilBlock *xs_lil_function_block_at(const XsLilFunction *function, size_t index);

XsLilStatus xs_lil_function_append_block(XsLilFunction *function, const char *label, XsLilBlock **block,
                                         XsLilError *error);
XsLilStatus xs_lil_block_add_const_i64(XsLilBlock *block, int64_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_i32(XsLilBlock *block, int32_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_u16(XsLilBlock *block, uint16_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_u8(XsLilBlock *block, uint8_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_i8(XsLilBlock *block, int8_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_i16(XsLilBlock *block, int16_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_u32(XsLilBlock *block, uint32_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_u64(XsLilBlock *block, uint64_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_u128(XsLilBlock *block, XsUInt128 value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_i128(XsLilBlock *block, XsInt128 value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_binary_integer(XsLilBlock *block, XsLilIntegerBinaryOperation operation, XsLilType type,
                                        XsLilValueId left, XsLilValueId right, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_bool(XsLilBlock *block, bool value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_str(XsLilBlock *block, XsLilUtf16Encoding encoding, const uint16_t *units,
                                       size_t unit_count, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_f32_bits(XsLilBlock *block, uint32_t bits, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_f64_bits(XsLilBlock *block, uint64_t bits, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_binary_float(XsLilBlock *block, XsLilFloatBinaryOperation operation, XsLilType type,
                                      XsLilValueId left, XsLilValueId right, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_compare_float(XsLilBlock *block, XsLilFloatComparisonOperation operation, XsLilType type,
                                       XsLilValueId left, XsLilValueId right, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_compare_str(XsLilBlock *block, XsLilStrComparisonOperation operation, XsLilValueId left,
                                     XsLilValueId right, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_sub_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_mul_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_div_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_rem_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_and_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_or_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_xor_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_shl_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_shr_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_eq_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_ne_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_lt_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_le_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_gt_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_ge_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_add_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_sub_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_mul_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_div_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_rem_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_and_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_or_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_xor_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_shl_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_shr_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_eq_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_ne_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_lt_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_le_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_gt_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_ge_i32(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                XsLilError *error);
XsLilStatus xs_lil_block_not_bool(XsLilBlock *block, XsLilValueId operand, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_call(XsLilBlock *block, const char *callee, XsLilType return_type,
                                  const XsLilValueId *arguments, size_t argument_count, XsLilValueId *result,
                                  XsLilError *error);
XsLilStatus xs_lil_block_add_void_call(XsLilBlock *block, const char *callee, const XsLilValueId *arguments,
                                       size_t argument_count, XsLilError *error);
XsLilStatus xs_lil_block_add_aggregate(XsLilBlock *block, XsLilType type, const XsLilValueId *fields,
                                       size_t field_count, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_array(XsLilBlock *block, XsLilType type, const XsLilValueId *elements,
                                   size_t element_count, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_extract(XsLilBlock *block, XsLilValueId aggregate, uint32_t field, XsLilType field_type,
                                     XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_array_get(XsLilBlock *block, XsLilValueId array, XsLilValueId index,
                                       XsLilType element_type, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_array_set(XsLilBlock *block, XsLilValueId array, XsLilValueId index,
                                       XsLilValueId value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_load(XsLilBlock *block, XsLilSlotId slot, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_store(XsLilBlock *block, XsLilSlotId slot, XsLilValueId value, XsLilError *error);
XsLilStatus xs_lil_block_set_return(XsLilBlock *block, XsLilError *error);
XsLilStatus xs_lil_block_set_return_value(XsLilBlock *block, XsLilValueId value, XsLilError *error);
XsLilStatus xs_lil_block_set_branch(XsLilBlock *block, XsLilBlockId target, XsLilError *error);
XsLilStatus xs_lil_block_set_branch_if(XsLilBlock *block, XsLilValueId condition, XsLilBlockId then_block,
                                       XsLilBlockId else_block, XsLilError *error);
XsLilStatus xs_lil_block_set_panic(XsLilBlock *block, XsLilError *error);
XsLilBlockId xs_lil_block_id(const XsLilBlock *block);
const char *xs_lil_block_label(const XsLilBlock *block);
size_t xs_lil_block_instruction_count(const XsLilBlock *block);
XsLilInstructionKind xs_lil_block_instruction_kind(const XsLilBlock *block, size_t index);
XsLilValueId xs_lil_block_instruction_result(const XsLilBlock *block, size_t index);
int64_t xs_lil_block_instruction_i64(const XsLilBlock *block, size_t index);
uint16_t xs_lil_block_instruction_u16(const XsLilBlock *block, size_t index);
XsUInt128 xs_lil_block_instruction_integer_bits(const XsLilBlock *block, size_t index);
XsLilIntegerBinaryOperation xs_lil_block_instruction_integer_operation(const XsLilBlock *block, size_t index);
XsLilType xs_lil_block_instruction_integer_type(const XsLilBlock *block, size_t index);
uint64_t xs_lil_block_instruction_float_bits(const XsLilBlock *block, size_t index);
bool xs_lil_block_instruction_bool(const XsLilBlock *block, size_t index);
XsLilUtf16Encoding xs_lil_block_instruction_utf16_encoding(const XsLilBlock *block, size_t index);
size_t xs_lil_block_instruction_utf16_length(const XsLilBlock *block, size_t index);
uint16_t xs_lil_block_instruction_utf16_unit(const XsLilBlock *block, size_t index, size_t unit);
const char *xs_lil_block_instruction_callee(const XsLilBlock *block, size_t index);
size_t xs_lil_block_instruction_argument_count(const XsLilBlock *block, size_t index);
XsLilValueId xs_lil_block_instruction_argument(const XsLilBlock *block, size_t index, size_t argument);
XsLilValueId xs_lil_block_instruction_left(const XsLilBlock *block, size_t index);
XsLilValueId xs_lil_block_instruction_right(const XsLilBlock *block, size_t index);
XsLilSlotId xs_lil_block_instruction_slot(const XsLilBlock *block, size_t index);
XsLilTerminatorKind xs_lil_block_terminator_kind(const XsLilBlock *block);
bool xs_lil_block_terminator_has_value(const XsLilBlock *block);
XsLilValueId xs_lil_block_terminator_value(const XsLilBlock *block);
XsLilBlockId xs_lil_block_terminator_target(const XsLilBlock *block);
XsLilValueId xs_lil_block_terminator_condition(const XsLilBlock *block);
XsLilBlockId xs_lil_block_terminator_then_block(const XsLilBlock *block);
XsLilBlockId xs_lil_block_terminator_else_block(const XsLilBlock *block);

const char *xs_lil_type_name(XsLilType type);
XsLilStatus xs_lil_module_write_text(const XsLilModule *module, FILE *stream, XsLilError *error);

#endif
