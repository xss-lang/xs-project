/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/mir/xlil_lowering.h"
#include "model_internal.h"

#include <stdio.h>
#include <stdlib.h>

static XsMirStatus set_error(XsMirError *error, XsMirStatus status, const char *message)
{
  if (error != NULL)
  {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == NULL ? "MIR to XLIL lowering error" : message);
  }
  return status;
}

XsMirStatus xs_lil_module_add_mir_function_declarations(XsLilModule *module, const XsMirModule *mir, XsMirError *error)
{
  if (module == NULL || mir == NULL)
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "valid XLIL and MIR modules are required");
  for (size_t i = 0; i < xs_mir_module_function_count(mir); ++i)
  {
    const XsMirFunction *function = xs_mir_module_function_at(mir, i);
    XsLilError lil_error = {0};
    XsLilStatus status = xs_lil_module_add_function(
        module, xs_mir_function_name(function), xs_mir_function_return_type(function),
        xs_mir_function_parameters(function), xs_mir_function_parameter_count(function), &lil_error);
    if (status == XS_LIL_ALLOCATION_FAILED)
      return set_error(error, XS_MIR_ALLOCATION_FAILED, lil_error.message);
    if (status != XS_LIL_OK)
      return set_error(error, XS_MIR_INVALID_ARGUMENT, lil_error.message);
  }
  return XS_MIR_OK;
}

static XsMirStatus map_lil_status(XsLilStatus status, const XsLilError *lil_error, XsMirError *error)
{
  if (status == XS_LIL_OK)
    return XS_MIR_OK;
  if (status == XS_LIL_ALLOCATION_FAILED)
    return set_error(error, XS_MIR_ALLOCATION_FAILED, lil_error->message);
  return set_error(error, XS_MIR_INVALID_ARGUMENT, lil_error->message);
}

static XsMirStatus lower_instruction(const XsMirInstruction *instruction, XsLilBlock *block, XsLilValueId *values,
                                     XsMirError *error)
{
  switch (instruction->kind)
  {
  case XS_MIR_INSTRUCTION_CONST_I64:
  {
    XsLilError lil_error = {0};
    XsLilStatus status =
        xs_lil_block_add_const_i64(block, instruction->immediate_i64, &values[instruction->result], &lil_error);
    return map_lil_status(status, &lil_error, error);
  }
  case XS_MIR_INSTRUCTION_ADD_I64:
  case XS_MIR_INSTRUCTION_LOAD:
  case XS_MIR_INSTRUCTION_STORE:
    return set_error(error, XS_MIR_UNSUPPORTED, "MIR to XLIL body lowering does not support this instruction yet");
  }
  return set_error(error, XS_MIR_INVALID_ARGUMENT, "unknown MIR instruction kind while lowering to XLIL");
}

static XsMirStatus lower_terminator(const XsMirFunction *function, const XsMirBlock *mir_block, XsLilBlock *xlil_block,
                                    const XsLilValueId *values, XsMirError *error)
{
  (void)function;
  switch (mir_block->terminator.kind)
  {
  case XS_MIR_TERMINATOR_RETURN:
  {
    XsLilError lil_error = {0};
    XsLilStatus status =
        mir_block->terminator.has_value
            ? xs_lil_block_set_return_value(xlil_block, values[mir_block->terminator.value], &lil_error)
            : xs_lil_block_set_return(xlil_block, &lil_error);
    return map_lil_status(status, &lil_error, error);
  }
  case XS_MIR_TERMINATOR_NONE:
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR block is missing a terminator");
  case XS_MIR_TERMINATOR_GOTO:
  case XS_MIR_TERMINATOR_BRANCH:
  case XS_MIR_TERMINATOR_UNREACHABLE:
    return set_error(error, XS_MIR_UNSUPPORTED, "MIR to XLIL body lowering supports only return terminators for now");
  }
  return set_error(error, XS_MIR_INVALID_ARGUMENT, "unknown MIR terminator kind while lowering to XLIL");
}

static XsMirStatus lower_body(const XsMirFunction *function, XsLilFunction *xlil_function, XsMirError *error)
{
  XsLilValueId *values = NULL;
  if (function->value_count != 0)
  {
    values = calloc(function->value_count, sizeof(*values));
    if (values == NULL)
      return set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while lowering MIR values to XLIL");
  }
  for (size_t block_index = 0; block_index < function->block_count; ++block_index)
  {
    const XsMirBlock *mir_block = function->blocks[block_index];
    XsLilBlock *xlil_block = NULL;
    XsLilError lil_error = {0};
    XsLilStatus lil_status = xs_lil_function_append_block(xlil_function, mir_block->label, &xlil_block, &lil_error);
    XsMirStatus status = map_lil_status(lil_status, &lil_error, error);
    if (status != XS_MIR_OK)
    {
      free(values);
      return status;
    }
    for (size_t instruction_index = 0; instruction_index < mir_block->instruction_count; ++instruction_index)
    {
      status = lower_instruction(&mir_block->instructions[instruction_index], xlil_block, values, error);
      if (status != XS_MIR_OK)
      {
        free(values);
        return status;
      }
    }
    status = lower_terminator(function, mir_block, xlil_block, values, error);
    if (status != XS_MIR_OK)
    {
      free(values);
      return status;
    }
  }
  free(values);
  return XS_MIR_OK;
}

XsMirStatus xs_lil_module_add_mir_function_bodies(XsLilModule *module, const XsMirModule *mir, XsMirError *error)
{
  if (module == NULL || mir == NULL)
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "valid XLIL and MIR modules are required");
  for (size_t i = 0; i < xs_mir_module_function_count(mir); ++i)
  {
    const XsMirFunction *function = xs_mir_module_function_at(mir, i);
    XsLilError lil_error = {0};
    XsLilFunction *xlil_function = NULL;
    XsLilStatus lil_status =
        function->is_definition
            ? xs_lil_module_add_function_definition(module, function->qualified_name, function->return_type,
                                                    function->parameters, function->parameter_count, &xlil_function,
                                                    &lil_error)
            : xs_lil_module_add_function(module, function->qualified_name, function->return_type, function->parameters,
                                         function->parameter_count, &lil_error);
    XsMirStatus status = map_lil_status(lil_status, &lil_error, error);
    if (status != XS_MIR_OK)
      return status;
    if (function->is_definition)
    {
      status = lower_body(function, xlil_function, error);
      if (status != XS_MIR_OK)
        return status;
    }
  }
  return XS_MIR_OK;
}
