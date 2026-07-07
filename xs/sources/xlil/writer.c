/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <stdio.h>

static XsLilStatus write_signature(FILE *stream, XsLilError *error, const XsLilFunction *function)
{
  if (fprintf(stream, "%s %s(", xs_lil_type_name(function->return_type), function->name) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL function signature");
  for (size_t parameter = 0; parameter < function->parameter_count; ++parameter)
  {
    if (parameter != 0 && xs_lil_write_checked(stream, error, ", ") != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
    if (xs_lil_write_checked(stream, error, xs_lil_type_name(function->parameters[parameter])) != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
  }
  if (xs_lil_write_checked(stream, error, ")") != XS_LIL_OK)
    return error == NULL ? XS_LIL_IO_ERROR : error->status;
  return XS_LIL_OK;
}

static XsLilStatus write_block(FILE *stream, XsLilError *error, const XsLilBlock *block)
{
  if (fprintf(stream, "bb%u %s:\n", block->id, block->label) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL block label");
  for (size_t i = 0; i < block->instruction_count; ++i)
  {
    const XsLilInstruction *instruction = &block->instructions[i];
    if (instruction->kind == XS_LIL_INSTRUCTION_CONST_I64 &&
        fprintf(stream, "  r%u = const.i64 %lld\n", instruction->result, (long long)instruction->immediate_i64) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL const.i64 instruction");
  }
  if (block->terminator.kind == XS_LIL_TERMINATOR_RETURN)
  {
    if (block->terminator.has_value)
    {
      if (fprintf(stream, "  return r%u\n", block->terminator.value) < 0)
        return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL return terminator");
    }
    else if (xs_lil_write_checked(stream, error, "  return\n") != XS_LIL_OK)
    {
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
    }
  }
  else if (block->terminator.kind == XS_LIL_TERMINATOR_NONE)
  {
    if (xs_lil_write_checked(stream, error, "  <missing terminator>\n") != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
  }
  return XS_LIL_OK;
}

XsLilStatus xs_lil_module_write_text(const XsLilModule *module, FILE *stream, XsLilError *error)
{
  xs_lil_clear_error(error);
  if (module == NULL || stream == NULL)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL module and stream are required");
  if (fprintf(stream, "xlil module %s\n", module->name) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL module header");
  for (size_t i = 0; i < module->function_count; ++i)
  {
    const XsLilFunction *function = &module->functions[i];
    if (xs_lil_write_checked(stream, error, function->is_definition ? "define " : "declare ") != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
    if (write_signature(stream, error, function) != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
    if (!function->is_definition)
    {
      if (xs_lil_write_checked(stream, error, "\n") != XS_LIL_OK)
        return error == NULL ? XS_LIL_IO_ERROR : error->status;
      continue;
    }
    if (xs_lil_write_checked(stream, error, " {\n") != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
    for (size_t block = 0; block < function->block_count; ++block)
    {
      if (write_block(stream, error, function->blocks[block]) != XS_LIL_OK)
        return error == NULL ? XS_LIL_IO_ERROR : error->status;
    }
    if (xs_lil_write_checked(stream, error, "}\n") != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
  }
  return XS_LIL_OK;
}
