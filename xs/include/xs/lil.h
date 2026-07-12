/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_LIL_H
#define XS_LIL_H

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
} XsLilTypeKind;

typedef struct
{
  XsLilTypeKind kind;
} XsLilType;

typedef uint32_t XsLilValueId;
typedef uint32_t XsLilBlockId;

typedef enum
{
  XS_LIL_INSTRUCTION_CONST_I64,
  XS_LIL_INSTRUCTION_CONST_I32,
  XS_LIL_INSTRUCTION_CONST_BOOL,
  XS_LIL_INSTRUCTION_ADD_I64,
  XS_LIL_INSTRUCTION_SUB_I64,
  XS_LIL_INSTRUCTION_MUL_I64,
  XS_LIL_INSTRUCTION_EQ_I64,
  XS_LIL_INSTRUCTION_ADD_I32,
  XS_LIL_INSTRUCTION_SUB_I32,
  XS_LIL_INSTRUCTION_MUL_I32,
  XS_LIL_INSTRUCTION_DIV_I32,
  XS_LIL_INSTRUCTION_REM_I32,
  XS_LIL_INSTRUCTION_EQ_I32,
  XS_LIL_INSTRUCTION_NE_I32,
  XS_LIL_INSTRUCTION_LT_I32,
  XS_LIL_INSTRUCTION_LE_I32,
  XS_LIL_INSTRUCTION_GT_I32,
  XS_LIL_INSTRUCTION_GE_I32,
  XS_LIL_INSTRUCTION_CALL,
} XsLilInstructionKind;

typedef enum
{
  XS_LIL_TERMINATOR_NONE,
  XS_LIL_TERMINATOR_RETURN,
  XS_LIL_TERMINATOR_BRANCH,
  XS_LIL_TERMINATOR_BRANCH_IF,
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
size_t xs_lil_function_block_count(const XsLilFunction *function);
const XsLilBlock *xs_lil_function_block_at(const XsLilFunction *function, size_t index);

XsLilStatus xs_lil_function_append_block(XsLilFunction *function, const char *label, XsLilBlock **block,
                                         XsLilError *error);
XsLilStatus xs_lil_block_add_const_i64(XsLilBlock *block, int64_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_i32(XsLilBlock *block, int32_t value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_const_bool(XsLilBlock *block, bool value, XsLilValueId *result, XsLilError *error);
XsLilStatus xs_lil_block_add_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_sub_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_mul_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
                                 XsLilError *error);
XsLilStatus xs_lil_block_eq_i64(XsLilBlock *block, XsLilValueId left, XsLilValueId right, XsLilValueId *result,
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
XsLilStatus xs_lil_block_add_call(XsLilBlock *block, const char *callee, XsLilType return_type,
                                  const XsLilValueId *arguments, size_t argument_count, XsLilValueId *result,
                                  XsLilError *error);
XsLilStatus xs_lil_block_add_void_call(XsLilBlock *block, const char *callee, const XsLilValueId *arguments,
                                       size_t argument_count, XsLilError *error);
XsLilStatus xs_lil_block_set_return(XsLilBlock *block, XsLilError *error);
XsLilStatus xs_lil_block_set_return_value(XsLilBlock *block, XsLilValueId value, XsLilError *error);
XsLilStatus xs_lil_block_set_branch(XsLilBlock *block, XsLilBlockId target, XsLilError *error);
XsLilStatus xs_lil_block_set_branch_if(XsLilBlock *block, XsLilValueId condition, XsLilBlockId then_block,
                                       XsLilBlockId else_block, XsLilError *error);
XsLilBlockId xs_lil_block_id(const XsLilBlock *block);
const char *xs_lil_block_label(const XsLilBlock *block);
size_t xs_lil_block_instruction_count(const XsLilBlock *block);
XsLilInstructionKind xs_lil_block_instruction_kind(const XsLilBlock *block, size_t index);
XsLilValueId xs_lil_block_instruction_result(const XsLilBlock *block, size_t index);
int64_t xs_lil_block_instruction_i64(const XsLilBlock *block, size_t index);
bool xs_lil_block_instruction_bool(const XsLilBlock *block, size_t index);
const char *xs_lil_block_instruction_callee(const XsLilBlock *block, size_t index);
size_t xs_lil_block_instruction_argument_count(const XsLilBlock *block, size_t index);
XsLilValueId xs_lil_block_instruction_argument(const XsLilBlock *block, size_t index, size_t argument);
XsLilValueId xs_lil_block_instruction_left(const XsLilBlock *block, size_t index);
XsLilValueId xs_lil_block_instruction_right(const XsLilBlock *block, size_t index);
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
