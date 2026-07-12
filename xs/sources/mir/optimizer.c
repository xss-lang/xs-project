/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/mir/optimizer.h"
#include "model_internal.h"

#include <stdlib.h>

static bool find_const_i64(const XsMirFunction *function, XsMirValueId value, int64_t *result)
{
  for(size_t block_index = 0; block_index < function->block_count; ++block_index)
  {
    const XsMirBlock *block = function->blocks[block_index];
    for(size_t instruction_index = 0; instruction_index < block->instruction_count; ++instruction_index)
    {
      const XsMirInstruction *instruction = &block->instructions[instruction_index];
      if(instruction->kind == XS_MIR_INSTRUCTION_CONST_I64 && instruction->result == value)
      {
        *result = instruction->immediate_i64;
        return true;
      }
    }
  }
  return false;
}

static void fold_function_constants(XsMirFunction *function)
{
  if(!function->is_definition)
    return;
  for(size_t block_index = 0; block_index < function->block_count; ++block_index)
  {
    XsMirBlock *block = function->blocks[block_index];
    for(size_t instruction_index = 0; instruction_index < block->instruction_count; ++instruction_index)
    {
      XsMirInstruction *instruction = &block->instructions[instruction_index];
      if(instruction->kind != XS_MIR_INSTRUCTION_ADD_I64)
        continue;
      int64_t left = 0;
      int64_t right = 0;
      if(!find_const_i64(function, instruction->operand_left, &left) ||
         !find_const_i64(function, instruction->operand_right, &right))
        continue;
      *instruction = (XsMirInstruction){
          .kind = XS_MIR_INSTRUCTION_CONST_I64,
          .result = instruction->result,
          .immediate_i64 = left + right,
      };
    }
  }
}

static void mark_reachable(const XsMirFunction *function, size_t index, bool *reachable)
{
  if(index >= function->block_count || reachable[index])
    return;
  reachable[index] = true;
  const XsMirBlock *block = function->blocks[index];
  if(block->terminator.kind == XS_MIR_TERMINATOR_GOTO)
  {
    mark_reachable(function, block->terminator.target, reachable);
  }
  else if(block->terminator.kind == XS_MIR_TERMINATOR_BRANCH)
  {
    mark_reachable(function, block->terminator.target, reachable);
    mark_reachable(function, block->terminator.else_target, reachable);
  }
}

static void remap_goto_targets(XsMirFunction *function, const XsMirBlockId *remap)
{
  for(size_t i = 0; i < function->block_count; ++i)
  {
    XsMirBlock *block = function->blocks[i];
    block->id = (XsMirBlockId)i;
    if(block->terminator.kind == XS_MIR_TERMINATOR_GOTO)
    {
      block->terminator.target = remap[block->terminator.target];
    }
    else if(block->terminator.kind == XS_MIR_TERMINATOR_BRANCH)
    {
      block->terminator.target = remap[block->terminator.target];
      block->terminator.else_target = remap[block->terminator.else_target];
    }
  }
}

static XsMirStatus optimize_function_cfg(XsMirFunction *function, XsMirError *error)
{
  if(!function->is_definition || function->block_count == 0)
    return XS_MIR_OK;
  bool *reachable = calloc(function->block_count, sizeof(*reachable));
  XsMirBlockId *remap = calloc(function->block_count, sizeof(*remap));
  if(reachable == nullptr || remap == nullptr)
  {
    free(reachable);
    free(remap);
    return xs_mir_set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while optimizing MIR CFG");
  }
  mark_reachable(function, 0, reachable);
  size_t write = 0;
  for(size_t read = 0; read < function->block_count; ++read)
  {
    if(reachable[read])
    {
      remap[read] = (XsMirBlockId)write;
      function->blocks[write++] = function->blocks[read];
      continue;
    }
    xs_mir_block_free(function->blocks[read]);
    free(function->blocks[read]);
  }
  function->block_count = write;
  remap_goto_targets(function, remap);
  free(reachable);
  free(remap);
  return XS_MIR_OK;
}

XsMirStatus xs_mir_optimize_module_cfg(XsMirModule *module, XsMirError *error)
{
  xs_mir_clear_error(error);
  if(module == nullptr)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR module is required");
  for(size_t i = 0; i < module->function_count; ++i)
  {
    XsMirStatus status = optimize_function_cfg(&module->functions[i], error);
    if(status != XS_MIR_OK)
      return status;
  }
  return XS_MIR_OK;
}

XsMirStatus xs_mir_optimize_module_constants(XsMirModule *module, XsMirError *error)
{
  xs_mir_clear_error(error);
  if(module == nullptr)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR module is required");
  for(size_t i = 0; i < module->function_count; ++i)
    fold_function_constants(&module->functions[i]);
  return XS_MIR_OK;
}
