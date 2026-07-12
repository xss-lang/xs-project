/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <stdio.h>

static XsLilStatus write_signature(FILE *stream, XsLilError *error, const XsLilFunction *function)
{
  if(fprintf(stream, "%s : (", function->name) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL function signature");
  for(size_t parameter = 0; parameter < function->parameter_count; ++parameter)
  {
    if(parameter != 0 && xs_lil_write_checked(stream, error, ", ") != XS_LIL_OK)
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
    if(xs_lil_write_checked(stream, error, xs_lil_type_name(function->parameters[parameter])) != XS_LIL_OK)
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
  }
  if(fprintf(stream, ") -> %s", xs_lil_type_name(function->return_type)) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL function signature");
  if(xs_lil_write_checked(stream, error, "\n") != XS_LIL_OK)
    return error == nullptr ? XS_LIL_IO_ERROR : error->status;
  return XS_LIL_OK;
}

static XsLilStatus write_block(FILE *stream, XsLilError *error, const XsLilBlock *block)
{
  if(fprintf(stream, "bb%u.%s:\n", block->id, block->label) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL block label");
  for(size_t i = 0; i < block->instruction_count; ++i)
  {
    const XsLilInstruction *instruction = &block->instructions[i];
    if(instruction->kind == XS_LIL_INSTRUCTION_CONST_I64 &&
       fprintf(stream, "  %%r%u:i64 = const %lld\n", instruction->result, (long long)instruction->immediate_i64) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL const.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_CONST_I32 &&
       fprintf(stream, "  %%r%u:i32 = const.i32 %d\n", instruction->result, (int)instruction->immediate_i64) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL const.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_CONST_BOOL &&
       fprintf(stream, "  %%r%u:bool = const.bool %s\n", instruction->result,
               instruction->immediate_bool ? "true" : "false") < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL const.bool instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_ADD_I64 &&
       fprintf(stream, "  %%r%u:i64 = add.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL add.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_SUB_I64 &&
       fprintf(stream, "  %%r%u:i64 = sub.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL sub.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_MUL_I64 &&
       fprintf(stream, "  %%r%u:i64 = mul.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL mul.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_DIV_I64 &&
       fprintf(stream, "  %%r%u:i64 = div.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL div.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_REM_I64 &&
       fprintf(stream, "  %%r%u:i64 = rem.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL rem.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_AND_I64 &&
       fprintf(stream, "  %%r%u:i64 = and.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL and.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_OR_I64 &&
       fprintf(stream, "  %%r%u:i64 = or.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL or.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_SHL_I64 &&
       fprintf(stream, "  %%r%u:i64 = shl.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL shl.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_SHR_I64 &&
       fprintf(stream, "  %%r%u:i64 = shr.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL shr.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_EQ_I64 &&
       fprintf(stream, "  %%r%u:bool = eq.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL eq.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_NE_I64 &&
       fprintf(stream, "  %%r%u:bool = ne.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL ne.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_LT_I64 &&
       fprintf(stream, "  %%r%u:bool = lt.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL lt.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_LE_I64 &&
       fprintf(stream, "  %%r%u:bool = le.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL le.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_GT_I64 &&
       fprintf(stream, "  %%r%u:bool = gt.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL gt.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_GE_I64 &&
       fprintf(stream, "  %%r%u:bool = ge.i64 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL ge.i64 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_ADD_I32 &&
       fprintf(stream, "  %%r%u:i32 = add.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL add.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_SUB_I32 &&
       fprintf(stream, "  %%r%u:i32 = sub.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL sub.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_MUL_I32 &&
       fprintf(stream, "  %%r%u:i32 = mul.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL mul.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_DIV_I32 &&
       fprintf(stream, "  %%r%u:i32 = div.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL div.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_REM_I32 &&
       fprintf(stream, "  %%r%u:i32 = rem.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL rem.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_AND_I32 &&
       fprintf(stream, "  %%r%u:i32 = and.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL and.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_OR_I32 &&
       fprintf(stream, "  %%r%u:i32 = or.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL or.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_SHL_I32 &&
       fprintf(stream, "  %%r%u:i32 = shl.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL shl.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_SHR_I32 &&
       fprintf(stream, "  %%r%u:i32 = shr.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL shr.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_EQ_I32 &&
       fprintf(stream, "  %%r%u:bool = eq.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL eq.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_NE_I32 &&
       fprintf(stream, "  %%r%u:bool = ne.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL ne.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_LT_I32 &&
       fprintf(stream, "  %%r%u:bool = lt.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL lt.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_LE_I32 &&
       fprintf(stream, "  %%r%u:bool = le.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL le.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_GT_I32 &&
       fprintf(stream, "  %%r%u:bool = gt.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL gt.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_GE_I32 &&
       fprintf(stream, "  %%r%u:bool = ge.i32 %%r%u, %%r%u\n", instruction->result, instruction->left,
               instruction->right) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL ge.i32 instruction");
    if(instruction->kind == XS_LIL_INSTRUCTION_CALL)
    {
      if(instruction->result != UINT32_MAX &&
         fprintf(stream, "  %%r%u:%s = ", instruction->result,
                 xs_lil_type_name(block->owner->values[instruction->result].type)) < 0)
        return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL call result");
      if(fprintf(stream, "%scall %s(", instruction->result == UINT32_MAX ? "  " : "", instruction->callee) < 0)
        return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL call instruction");
      for(size_t argument = 0; argument < instruction->argument_count; ++argument)
      {
        if((argument != 0 && xs_lil_write_checked(stream, error, ", ") != XS_LIL_OK) ||
           fprintf(stream, "%%r%u", instruction->arguments[argument]) < 0)
          return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL call argument");
      }
      if(xs_lil_write_checked(stream, error, ")\n") != XS_LIL_OK)
        return error == nullptr ? XS_LIL_IO_ERROR : error->status;
    }
  }
  if(block->terminator.kind == XS_LIL_TERMINATOR_RETURN)
  {
    if(block->terminator.has_value)
    {
      if(fprintf(stream, "  ret %%r%u\n", block->terminator.value) < 0)
        return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL return terminator");
    }
    else if(xs_lil_write_checked(stream, error, "  ret\n") != XS_LIL_OK)
    {
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
    }
  }
  else if(block->terminator.kind == XS_LIL_TERMINATOR_BRANCH)
  {
    if(fprintf(stream, "  br bb%u\n", block->terminator.target) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL branch terminator");
  }
  else if(block->terminator.kind == XS_LIL_TERMINATOR_BRANCH_IF)
  {
    if(fprintf(stream, "  br_if %%r%u, bb%u, bb%u\n", block->terminator.condition, block->terminator.target,
               block->terminator.else_target) < 0)
      return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL branch_if terminator");
  }
  else if(block->terminator.kind == XS_LIL_TERMINATOR_NONE)
  {
    if(xs_lil_write_checked(stream, error, "  .missing_terminator\n") != XS_LIL_OK)
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
  }
  return XS_LIL_OK;
}

XsLilStatus xs_lil_module_write_text(const XsLilModule *module, FILE *stream, XsLilError *error)
{
  xs_lil_clear_error(error);
  if(module == nullptr || stream == nullptr)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL module and stream are required");
  if(xs_lil_write_checked(stream, error, ".xlil version 0\n") != XS_LIL_OK)
    return error == nullptr ? XS_LIL_IO_ERROR : error->status;
  if(fprintf(stream, ".xlil module %s\n", module->name) < 0)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL module header");
  for(size_t i = 0; i < module->function_count; ++i)
  {
    const XsLilFunction *function = &module->functions[i];
    if(xs_lil_write_checked(stream, error, function->is_definition ? ".func " : ".extern ") != XS_LIL_OK)
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
    if(write_signature(stream, error, function) != XS_LIL_OK)
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
    if(!function->is_definition)
      continue;
    for(size_t parameter = 0; parameter < function->parameter_count; ++parameter)
    {
      if(fprintf(stream, ".param %%r%zu:%s\n", parameter, xs_lil_type_name(function->parameters[parameter])) < 0)
        return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL function parameter");
    }
    for(size_t block = 0; block < function->block_count; ++block)
    {
      if(write_block(stream, error, function->blocks[block]) != XS_LIL_OK)
        return error == nullptr ? XS_LIL_IO_ERROR : error->status;
    }
    if(xs_lil_write_checked(stream, error, ".end\n") != XS_LIL_OK)
      return error == nullptr ? XS_LIL_IO_ERROR : error->status;
  }
  return XS_LIL_OK;
}
