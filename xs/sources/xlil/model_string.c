/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <stdlib.h>
#include <string.h>

static bool valid_utf16(const uint16_t *units, size_t length)
{
  for(size_t index = 0; index < length; ++index)
  {
    uint16_t unit = units[index];
    if(unit >= 0xD800U && unit <= 0xDBFFU)
    {
      if(++index >= length || units[index] < 0xDC00U || units[index] > 0xDFFFU)
        return false;
    }
    else if(unit >= 0xDC00U && unit <= 0xDFFFU)
      return false;
  }
  return true;
}

XsLilStatus xs_lil_block_add_const_str(XsLilBlock *block, XsLilUtf16Encoding encoding, const uint16_t *units,
                                       size_t unit_count, XsLilValueId *result, XsLilError *error)
{
  xs_lil_clear_error(error);
  if(result != nullptr)
    *result = 0;
  if(block == nullptr || block->owner == nullptr || (unit_count != 0 && units == nullptr) ||
     (encoding != XS_LIL_UTF16_LE && encoding != XS_LIL_UTF16_BE))
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL const.str arguments are required");
  if(!valid_utf16(units, unit_count))
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL const.str must contain well-formed UTF-16");
  uint16_t *copy = nullptr;
  if(unit_count != 0)
  {
    copy = malloc(unit_count * sizeof(*copy));
    if(copy == nullptr)
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL const.str");
    memcpy(copy, units, unit_count * sizeof(*copy));
  }
  XsLilValueId value = 0;
  XsLilStatus status = xs_lil_add_value(block->owner, (XsLilType){.kind = XS_LIL_TYPE_STR}, &value, error);
  if(status == XS_LIL_OK)
    status = xs_lil_append_instruction(block,
                                       (XsLilInstruction){.kind = XS_LIL_INSTRUCTION_CONST_STR,
                                                          .result = value,
                                                          .utf16_encoding = encoding,
                                                          .utf16_units = copy,
                                                          .utf16_length = unit_count},
                                       error);
  if(status != XS_LIL_OK)
  {
    free(copy);
    return status;
  }
  if(result != nullptr)
    *result = value;
  return XS_LIL_OK;
}

XsLilUtf16Encoding xs_lil_block_instruction_utf16_encoding(const XsLilBlock *block, size_t index)
{
  if(block == nullptr || index >= block->instruction_count ||
     block->instructions[index].kind != XS_LIL_INSTRUCTION_CONST_STR)
    return XS_LIL_UTF16_LE;
  return block->instructions[index].utf16_encoding;
}

size_t xs_lil_block_instruction_utf16_length(const XsLilBlock *block, size_t index)
{
  if(block == nullptr || index >= block->instruction_count ||
     block->instructions[index].kind != XS_LIL_INSTRUCTION_CONST_STR)
    return 0;
  return block->instructions[index].utf16_length;
}

uint16_t xs_lil_block_instruction_utf16_unit(const XsLilBlock *block, size_t index, size_t unit)
{
  if(block == nullptr || index >= block->instruction_count ||
     block->instructions[index].kind != XS_LIL_INSTRUCTION_CONST_STR || unit >= block->instructions[index].utf16_length)
    return 0;
  return block->instructions[index].utf16_units[unit];
}
