/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/mir/borrow_checker.h"
#include "model_internal.h"

static bool type_equal(XsMirType left, XsMirType right)
{
  return left.kind == right.kind;
}

static bool value_exists(const XsMirFunction *function, XsMirValueId value)
{
  return (size_t)value < function->value_count;
}

static XsMirStatus check_value_exists(const XsMirFunction *function, XsMirValueId value, const char *message,
                                      XsMirError *error)
{
  if (!value_exists(function, value))
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, message);
  return XS_MIR_OK;
}

static XsMirStatus find_place(const XsMirFunction *function, XsMirPlaceId place_id, const XsMirPlace **place,
                              XsMirError *error)
{
  if ((size_t)place_id >= function->place_count || function->places[place_id] == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR instruction references an unknown place");
  *place = function->places[place_id];
  if ((size_t)(*place)->root_local >= function->local_count)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR place references an unknown local");
  return XS_MIR_OK;
}

static XsMirStatus check_store_mutability(const XsMirFunction *function, const XsMirPlace *place, XsMirError *error)
{
  if (place == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR store references an unknown place");
  const XsMirLocal *local = &function->locals[place->root_local];
  if (!local->is_mutable)
    return xs_mir_set_error(error, XS_MIR_UNSUPPORTED, "MIR store targets an immutable local");
  return XS_MIR_OK;
}

static XsMirStatus check_i64_operand(const XsMirFunction *function, XsMirValueId value, const char *message,
                                     XsMirError *error)
{
  XsMirStatus status = check_value_exists(function, value, message, error);
  if (status != XS_MIR_OK)
    return status;
  if (!type_equal(function->values[value].type, (XsMirType){.kind = XS_LIL_TYPE_I64}))
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR i64 instruction operand is not i64");
  return XS_MIR_OK;
}

static XsMirStatus check_instruction(const XsMirFunction *function, const XsMirInstruction *instruction,
                                     XsMirError *error)
{
  switch (instruction->kind)
  {
  case XS_MIR_INSTRUCTION_CONST_I64:
    return check_value_exists(function, instruction->result, "MIR const.i64 result is unknown", error);
  case XS_MIR_INSTRUCTION_ADD_I64:
  {
    XsMirStatus status = check_value_exists(function, instruction->result, "MIR add.i64 result is unknown", error);
    if (status != XS_MIR_OK)
      return status;
    status = check_i64_operand(function, instruction->operand_left, "MIR add.i64 left operand is unknown", error);
    if (status != XS_MIR_OK)
      return status;
    return check_i64_operand(function, instruction->operand_right, "MIR add.i64 right operand is unknown", error);
  }
  case XS_MIR_INSTRUCTION_LOAD:
  {
    const XsMirPlace *place = NULL;
    XsMirStatus status = check_value_exists(function, instruction->result, "MIR load result is unknown", error);
    if (status != XS_MIR_OK)
      return status;
    return find_place(function, instruction->place, &place, error);
  }
  case XS_MIR_INSTRUCTION_STORE:
  {
    const XsMirPlace *place = NULL;
    XsMirStatus status = check_value_exists(function, instruction->operand_left, "MIR store value is unknown", error);
    if (status != XS_MIR_OK)
      return status;
    status = find_place(function, instruction->place, &place, error);
    if (status != XS_MIR_OK)
      return status;
    return check_store_mutability(function, place, error);
  }
  }
  return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR instruction has an unknown kind");
}

static XsMirStatus check_return_type(const XsMirFunction *function, const XsMirBlock *block, XsMirError *error)
{
  if (block->terminator.kind != XS_MIR_TERMINATOR_RETURN)
    return XS_MIR_OK;
  if (!block->terminator.has_value)
    return type_equal(function->return_type, (XsMirType){.kind = XS_LIL_TYPE_VOID})
               ? XS_MIR_OK
               : xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR return is missing a value");
  if ((size_t)block->terminator.value >= function->value_count)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR return references an unknown value");
  if (!type_equal(function->values[block->terminator.value].type, function->return_type))
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT,
                            "MIR return value type does not match function return type");
  return XS_MIR_OK;
}

static XsMirStatus check_terminator(const XsMirFunction *function, const XsMirBlock *block, XsMirError *error)
{
  switch (block->terminator.kind)
  {
  case XS_MIR_TERMINATOR_NONE:
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR block is missing a terminator");
  case XS_MIR_TERMINATOR_RETURN:
    return check_return_type(function, block, error);
  case XS_MIR_TERMINATOR_GOTO:
    if ((size_t)block->terminator.target >= function->block_count)
      return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR goto references an unknown block");
    return XS_MIR_OK;
  case XS_MIR_TERMINATOR_BRANCH:
  {
    XsMirStatus status =
        check_value_exists(function, block->terminator.value, "MIR branch condition is unknown", error);
    if (status != XS_MIR_OK)
      return status;
    if (!type_equal(function->values[block->terminator.value].type, (XsMirType){.kind = XS_LIL_TYPE_BOOL}))
      return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR branch condition is not bool");
    if ((size_t)block->terminator.target >= function->block_count ||
        (size_t)block->terminator.else_target >= function->block_count)
      return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR branch references an unknown block");
    return XS_MIR_OK;
  }
  case XS_MIR_TERMINATOR_UNREACHABLE:
    return XS_MIR_OK;
  }
  return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR terminator has an unknown kind");
}

static XsMirStatus check_block(const XsMirFunction *function, const XsMirBlock *block, XsMirError *error)
{
  for (size_t i = 0; i < block->instruction_count; ++i)
  {
    XsMirStatus status = check_instruction(function, &block->instructions[i], error);
    if (status != XS_MIR_OK)
      return status;
  }
  return check_terminator(function, block, error);
}

static XsMirStatus check_function(const XsMirFunction *function, XsMirError *error)
{
  if (!function->is_definition)
    return XS_MIR_OK;
  for (size_t i = 0; i < function->block_count; ++i)
  {
    XsMirStatus status = check_block(function, function->blocks[i], error);
    if (status != XS_MIR_OK)
      return status;
  }
  return XS_MIR_OK;
}

XsMirStatus xs_mir_borrow_check_module(const XsMirModule *module, XsMirError *error)
{
  xs_mir_clear_error(error);
  if (module == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR module is required");
  for (size_t i = 0; i < module->function_count; ++i)
  {
    XsMirStatus status = check_function(&module->functions[i], error);
    if (status != XS_MIR_OK)
      return status;
  }
  return XS_MIR_OK;
}
