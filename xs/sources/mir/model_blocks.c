#include "model_internal.h"

#include <stdlib.h>

XsMirStatus xs_mir_function_append_block(XsMirFunction *function, const char *label, XsMirBlock **block,
                                         XsMirError *error)
{
  xs_mir_clear_error(error);
  if (block != NULL)
    *block = NULL;
  if (function == NULL || !function->is_definition || label == NULL || label[0] == '\0')
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT,
                            "valid MIR function definition and block label are required");
  if (function->block_count == function->block_capacity)
  {
    size_t capacity = function->block_capacity == 0 ? 4 : function->block_capacity * 2;
    XsMirBlock **blocks = realloc(function->blocks, capacity * sizeof(*blocks));
    if (blocks == NULL)
      return xs_mir_set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while adding MIR block");
    function->blocks = blocks;
    function->block_capacity = capacity;
  }
  XsMirBlock *created = calloc(1, sizeof(*created));
  if (created == NULL)
    return xs_mir_set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while adding MIR block");
  *created = (XsMirBlock){
      .label = xs_mir_copy_text(label),
      .owner = function,
      .id = (XsMirBlockId)function->block_count,
  };
  if (created->label == NULL)
  {
    free(created);
    return xs_mir_set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while naming MIR block");
  }
  function->blocks[function->block_count] = created;
  if (block != NULL)
    *block = created;
  ++function->block_count;
  return XS_MIR_OK;
}

size_t xs_mir_function_block_count(const XsMirFunction *function)
{
  return function == NULL ? 0 : function->block_count;
}

const XsMirBlock *xs_mir_function_block_at(const XsMirFunction *function, size_t index)
{
  if (function == NULL || index >= function->block_count)
    return NULL;
  return function->blocks[index];
}

XsMirBlockId xs_mir_block_id(const XsMirBlock *block)
{
  return block == NULL ? 0 : block->id;
}

const char *xs_mir_block_label(const XsMirBlock *block)
{
  return block == NULL ? NULL : block->label;
}

XsMirTerminatorKind xs_mir_block_terminator_kind(const XsMirBlock *block)
{
  return block == NULL ? XS_MIR_TERMINATOR_NONE : block->terminator.kind;
}

static XsMirStatus set_terminator(XsMirBlock *block, XsMirTerminator terminator, XsMirError *error)
{
  xs_mir_clear_error(error);
  if (block == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR block is required");
  if (block->terminator.kind != XS_MIR_TERMINATOR_NONE)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR block already has a terminator");
  block->terminator = terminator;
  return XS_MIR_OK;
}

XsMirStatus xs_mir_block_set_return(XsMirBlock *block, XsMirError *error)
{
  return set_terminator(block, (XsMirTerminator){.kind = XS_MIR_TERMINATOR_RETURN}, error);
}

XsMirStatus xs_mir_block_set_return_value(XsMirBlock *block, XsMirValueId value, XsMirError *error)
{
  return set_terminator(block, (XsMirTerminator){.kind = XS_MIR_TERMINATOR_RETURN, .has_value = true, .value = value},
                        error);
}

XsMirStatus xs_mir_block_set_goto(XsMirBlock *block, const XsMirBlock *target, XsMirError *error)
{
  if (target == NULL)
  {
    xs_mir_clear_error(error);
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR goto target is required");
  }
  return set_terminator(block, (XsMirTerminator){.kind = XS_MIR_TERMINATOR_GOTO, .target = target->id}, error);
}

XsMirStatus xs_mir_block_set_branch(XsMirBlock *block, XsMirValueId condition, const XsMirBlock *then_target,
                                    const XsMirBlock *else_target, XsMirError *error)
{
  if (then_target == NULL || else_target == NULL)
  {
    xs_mir_clear_error(error);
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR branch targets are required");
  }
  return set_terminator(block,
                        (XsMirTerminator){
                            .kind = XS_MIR_TERMINATOR_BRANCH,
                            .has_value = true,
                            .value = condition,
                            .target = then_target->id,
                            .else_target = else_target->id,
                        },
                        error);
}

XsMirStatus xs_mir_block_set_unreachable(XsMirBlock *block, XsMirError *error)
{
  return set_terminator(block, (XsMirTerminator){.kind = XS_MIR_TERMINATOR_UNREACHABLE}, error);
}

static XsMirStatus append_instruction(XsMirBlock *block, XsMirInstruction instruction, XsMirError *error)
{
  xs_mir_clear_error(error);
  if (block == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR block is required");
  if (block->terminator.kind != XS_MIR_TERMINATOR_NONE)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "cannot add MIR instruction after terminator");
  if (block->instruction_count == block->instruction_capacity)
  {
    size_t capacity = block->instruction_capacity == 0 ? 8 : block->instruction_capacity * 2;
    XsMirInstruction *instructions = realloc(block->instructions, capacity * sizeof(*instructions));
    if (instructions == NULL)
      return xs_mir_set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while adding MIR instruction");
    block->instructions = instructions;
    block->instruction_capacity = capacity;
  }
  block->instructions[block->instruction_count++] = instruction;
  return XS_MIR_OK;
}

XsMirStatus xs_mir_block_add_const_i64(XsMirBlock *block, int64_t value, XsMirValueId *result, XsMirError *error)
{
  xs_mir_clear_error(error);
  if (result != NULL)
    *result = 0;
  if (block == NULL || block->owner == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR block is required");
  XsMirValueId value_id = 0;
  XsMirStatus status = xs_mir_function_add_value(block->owner, (XsMirType){.kind = XS_LIL_TYPE_I64}, &value_id, error);
  if (status != XS_MIR_OK)
    return status;
  status = append_instruction(block,
                              (XsMirInstruction){
                                  .kind = XS_MIR_INSTRUCTION_CONST_I64,
                                  .result = value_id,
                                  .immediate_i64 = value,
                              },
                              error);
  if (status != XS_MIR_OK)
    return status;
  if (result != NULL)
    *result = value_id;
  return XS_MIR_OK;
}

XsMirStatus xs_mir_block_add_i64(XsMirBlock *block, XsMirValueId left, XsMirValueId right, XsMirValueId *result,
                                 XsMirError *error)
{
  xs_mir_clear_error(error);
  if (result != NULL)
    *result = 0;
  if (block == NULL || block->owner == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR block is required");
  XsMirValueId value_id = 0;
  XsMirStatus status = xs_mir_function_add_value(block->owner, (XsMirType){.kind = XS_LIL_TYPE_I64}, &value_id, error);
  if (status != XS_MIR_OK)
    return status;
  status = append_instruction(block,
                              (XsMirInstruction){
                                  .kind = XS_MIR_INSTRUCTION_ADD_I64,
                                  .result = value_id,
                                  .operand_left = left,
                                  .operand_right = right,
                              },
                              error);
  if (status != XS_MIR_OK)
    return status;
  if (result != NULL)
    *result = value_id;
  return XS_MIR_OK;
}

XsMirStatus xs_mir_block_add_load(XsMirBlock *block, const XsMirPlace *place, XsMirType result_type,
                                  XsMirValueId *result, XsMirError *error)
{
  xs_mir_clear_error(error);
  if (result != NULL)
    *result = 0;
  if (block == NULL || block->owner == NULL || place == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR block and place are required");
  XsMirValueId value_id = 0;
  XsMirStatus status = xs_mir_function_add_value(block->owner, result_type, &value_id, error);
  if (status != XS_MIR_OK)
    return status;
  status = append_instruction(block,
                              (XsMirInstruction){
                                  .kind = XS_MIR_INSTRUCTION_LOAD,
                                  .result = value_id,
                                  .place = place->id,
                              },
                              error);
  if (status != XS_MIR_OK)
    return status;
  if (result != NULL)
    *result = value_id;
  return XS_MIR_OK;
}

XsMirStatus xs_mir_block_add_store(XsMirBlock *block, const XsMirPlace *place, XsMirValueId value, XsMirError *error)
{
  if (block == NULL || place == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR block and place are required");
  return append_instruction(block,
                            (XsMirInstruction){
                                .kind = XS_MIR_INSTRUCTION_STORE,
                                .operand_left = value,
                                .place = place->id,
                            },
                            error);
}

size_t xs_mir_block_instruction_count(const XsMirBlock *block)
{
  return block == NULL ? 0 : block->instruction_count;
}

XsMirInstructionKind xs_mir_block_instruction_kind(const XsMirBlock *block, size_t index)
{
  if (block == NULL || index >= block->instruction_count)
    return XS_MIR_INSTRUCTION_CONST_I64;
  return block->instructions[index].kind;
}
