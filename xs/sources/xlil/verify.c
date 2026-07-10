/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <string.h>

static const XsLilFunction *find_function(const XsLilModule *module, const char *name, bool *duplicate)
{
  const XsLilFunction *result = nullptr;
  *duplicate = false;
  for (size_t index = 0; index < module->function_count; ++index)
  {
    const XsLilFunction *function = &module->functions[index];
    if (strcmp(function->name, name) != 0)
      continue;
    if (result != nullptr)
    {
      *duplicate = true;
      return nullptr;
    }
    result = function;
  }
  return result;
}

static XsLilStatus verify_parameters(const XsLilFunction *function, XsLilError *error)
{
  if (function->value_count < function->parameter_count)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL function parameter values are missing");
  for (size_t parameter = 0; parameter < function->parameter_count; ++parameter)
  {
    if (function->parameters[parameter].kind == XS_LIL_TYPE_VOID ||
        function->values[parameter].type.kind != function->parameters[parameter].kind)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL function parameter type is invalid");
  }
  return XS_LIL_OK;
}

static XsLilStatus verify_call(const XsLilModule *module, const XsLilFunction *function,
                               const XsLilInstruction *instruction, XsLilError *error)
{
  bool duplicate = false;
  const XsLilFunction *callee = find_function(module, instruction->callee, &duplicate);
  if (duplicate)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL call target is declared more than once");
  if (callee == nullptr)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL call target is not declared in this module");
  if (instruction->argument_count != callee->parameter_count)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL call argument count does not match target signature");
  for (size_t argument = 0; argument < instruction->argument_count; ++argument)
  {
    XsLilValueId value = instruction->arguments[argument];
    if ((size_t)value >= function->value_count ||
        function->values[value].type.kind != callee->parameters[argument].kind)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT,
                              "XLIL call argument type does not match target signature");
  }
  if (callee->return_type.kind == XS_LIL_TYPE_VOID)
  {
    if (instruction->result != UINT32_MAX)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "void XLIL call cannot produce a value");
    return XS_LIL_OK;
  }
  if ((size_t)instruction->result >= function->value_count ||
      function->values[instruction->result].type.kind != callee->return_type.kind)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL call result type does not match target signature");
  return XS_LIL_OK;
}

static bool is_binary_i64(XsLilInstructionKind kind)
{
  return kind == XS_LIL_INSTRUCTION_ADD_I64 || kind == XS_LIL_INSTRUCTION_SUB_I64 ||
         kind == XS_LIL_INSTRUCTION_MUL_I64 || kind == XS_LIL_INSTRUCTION_EQ_I64;
}

static bool is_binary_i32(XsLilInstructionKind kind)
{
  return kind == XS_LIL_INSTRUCTION_ADD_I32 || kind == XS_LIL_INSTRUCTION_SUB_I32 ||
         kind == XS_LIL_INSTRUCTION_MUL_I32 || kind == XS_LIL_INSTRUCTION_EQ_I32;
}

static XsLilStatus verify_binary_integer(const XsLilFunction *function, const XsLilInstruction *instruction,
                                         XsLilError *error)
{
  XsLilTypeKind operand = is_binary_i32(instruction->kind) ? XS_LIL_TYPE_I32 : XS_LIL_TYPE_I64;
  if ((size_t)instruction->left >= function->value_count || (size_t)instruction->right >= function->value_count ||
      (size_t)instruction->result >= function->value_count ||
      function->values[instruction->left].type.kind != operand ||
      function->values[instruction->right].type.kind != operand)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL binary integer instruction has invalid operands");
  XsLilTypeKind expected =
      instruction->kind == XS_LIL_INSTRUCTION_EQ_I64 || instruction->kind == XS_LIL_INSTRUCTION_EQ_I32
          ? XS_LIL_TYPE_BOOL
          : operand;
  if (function->values[instruction->result].type.kind != expected)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT,
                            "XLIL binary integer instruction has an invalid result type");
  return XS_LIL_OK;
}

XsLilStatus xs_lil_module_verify(const XsLilModule *module, XsLilError *error)
{
  xs_lil_clear_error(error);
  if (module == nullptr)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL module is required");
  for (size_t index = 0; index < module->function_count; ++index)
  {
    const XsLilFunction *function = &module->functions[index];
    XsLilStatus status = verify_parameters(function, error);
    if (status != XS_LIL_OK)
      return status;
    if (function->is_definition && function->block_count == 0)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL function definition requires a block");
    for (size_t block_index = 0; block_index < function->block_count; ++block_index)
    {
      const XsLilBlock *block = function->blocks[block_index];
      if (block->terminator.kind == XS_LIL_TERMINATOR_NONE)
        return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL block is missing a terminator");
      for (size_t instruction = 0; instruction < block->instruction_count; ++instruction)
      {
        const XsLilInstruction *current = &block->instructions[instruction];
        if (current->kind == XS_LIL_INSTRUCTION_CALL)
        {
          status = verify_call(module, function, current, error);
          if (status != XS_LIL_OK)
            return status;
        }
        if (is_binary_i64(current->kind) || is_binary_i32(current->kind))
        {
          status = verify_binary_integer(function, current, error);
          if (status != XS_LIL_OK)
            return status;
        }
      }
    }
  }
  return XS_LIL_OK;
}
