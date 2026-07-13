/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_XLIL_MODEL_INTERNAL_H
#define XS_XLIL_MODEL_INTERNAL_H

#include "xs/lil.h"

typedef struct
{
  XsLilTerminatorKind kind;
  bool has_value;
  XsLilValueId value;
  XsLilValueId condition;
  XsLilBlockId target;
  XsLilBlockId else_target;
} XsLilTerminator;

typedef struct
{
  XsLilType type;
} XsLilValue;

typedef struct
{
  XsLilType type;
} XsLilSlot;

typedef struct
{
  XsLilInstructionKind kind;
  XsLilValueId result;
  int64_t immediate_i64;
  uint64_t immediate_float_bits;
  bool immediate_bool;
  XsLilUtf16Encoding utf16_encoding;
  uint16_t *utf16_units;
  size_t utf16_length;
  XsLilValueId left;
  XsLilValueId right;
  XsLilSlotId slot;
  char *callee;
  XsLilValueId *arguments;
  size_t argument_count;
} XsLilInstruction;

struct XsLilBlock
{
  char *label;
  XsLilFunction *owner;
  uint32_t id;
  XsLilInstruction *instructions;
  size_t instruction_count;
  size_t instruction_capacity;
  XsLilTerminator terminator;
};

struct XsLilFunction
{
  char *name;
  XsLilType return_type;
  XsLilType *parameters;
  size_t parameter_count;
  bool is_definition;
  XsLilValue *values;
  size_t value_count;
  size_t value_capacity;
  XsLilSlot *slots;
  size_t slot_count;
  size_t slot_capacity;
  XsLilBlock **blocks;
  size_t block_count;
  size_t block_capacity;
};

struct XsLilModule
{
  char *name;
  XsLilFunction *functions;
  size_t function_count;
  size_t function_capacity;
};

void xs_lil_clear_error(XsLilError *error);
XsLilStatus xs_lil_set_error(XsLilError *error, XsLilStatus status, const char *message);
char *xs_lil_copy_text(const char *text);
char *xs_lil_copy_span(const char *text, size_t length);
XsLilStatus xs_lil_write_checked(FILE *stream, XsLilError *error, const char *text);
XsLilStatus xs_lil_add_value(XsLilFunction *function, XsLilType type, XsLilValueId *value, XsLilError *error);
XsLilStatus xs_lil_append_instruction(XsLilBlock *block, XsLilInstruction instruction, XsLilError *error);
XsLilStatus xs_lil_parse_const_str(XsLilBlock *block, XsLilType result_type, const char *operation,
                                   size_t operation_length, XsLilValueId expected_result, bool *matched,
                                   XsLilError *error);
XsLilStatus xs_lil_parse_const_u16(XsLilBlock *block, XsLilType result_type, const char *operation,
                                   size_t operation_length, XsLilValueId expected_result, bool *matched,
                                   XsLilError *error);

#endif
