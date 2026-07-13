/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

XsLilStatus xs_lil_block_add_const_u16(XsLilBlock *block, uint16_t value, XsLilValueId *result, XsLilError *error)
{
  xs_lil_clear_error(error);
  if(result != nullptr)
    *result = 0;
  if(block == nullptr || block->owner == nullptr)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL block is required");
  XsLilValueId value_id = 0;
  XsLilStatus status = xs_lil_add_value(block->owner, (XsLilType){.kind = XS_LIL_TYPE_U16}, &value_id, error);
  if(status == XS_LIL_OK)
    status = xs_lil_append_instruction(
        block, (XsLilInstruction){.kind = XS_LIL_INSTRUCTION_CONST_U16, .result = value_id, .immediate_i64 = value},
        error);
  if(status != XS_LIL_OK)
    return status;
  if(result != nullptr)
    *result = value_id;
  return XS_LIL_OK;
}

uint16_t xs_lil_block_instruction_u16(const XsLilBlock *block, size_t index)
{
  if(block == nullptr || index >= block->instruction_count ||
     block->instructions[index].kind != XS_LIL_INSTRUCTION_CONST_U16)
    return 0;
  return (uint16_t)block->instructions[index].immediate_i64;
}
