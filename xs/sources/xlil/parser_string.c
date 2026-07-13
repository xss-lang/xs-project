/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool append_unit(uint16_t **units, size_t *length, size_t *capacity, uint16_t unit)
{
  if(*length == *capacity)
  {
    size_t next = *capacity == 0 ? 8 : *capacity * 2;
    uint16_t *grown = realloc(*units, next * sizeof(*grown));
    if(grown == nullptr)
      return false;
    *units = grown;
    *capacity = next;
  }
  (*units)[(*length)++] = unit;
  return true;
}

XsLilStatus xs_lil_parse_const_str(XsLilBlock *block, XsLilType result_type, const char *operation,
                                   size_t operation_length, XsLilValueId expected_result, bool *matched,
                                   XsLilError *error)
{
  static const char le[] = "const.str utf16le [";
  static const char be[] = "const.str utf16be [";
  *matched = false;
  XsLilUtf16Encoding encoding = XS_LIL_UTF16_LE;
  size_t prefix = 0;
  if(operation_length >= sizeof(le) - 1U && memcmp(operation, le, sizeof(le) - 1U) == 0)
    prefix = sizeof(le) - 1U;
  else if(operation_length >= sizeof(be) - 1U && memcmp(operation, be, sizeof(be) - 1U) == 0)
  {
    prefix = sizeof(be) - 1U;
    encoding = XS_LIL_UTF16_BE;
  }
  else
    return XS_LIL_OK;
  *matched = true;
  if(result_type.kind != XS_LIL_TYPE_STR || operation_length <= prefix || operation[operation_length - 1] != ']')
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL const.str record is invalid");
  const char *cursor = operation + prefix;
  const char *end = operation + operation_length - 1;
  uint16_t *units = nullptr;
  size_t length = 0;
  size_t capacity = 0;
  while(cursor < end)
  {
    if(end - cursor < 6 || cursor[0] != '0' || cursor[1] != 'x')
      goto invalid;
    char digits[5] = {cursor[2], cursor[3], cursor[4], cursor[5], '\0'};
    char *tail = nullptr;
    errno = 0;
    unsigned long value = strtoul(digits, &tail, 16);
    if(errno != 0 || tail == digits || *tail != '\0')
      goto invalid;
    if(!append_unit(&units, &length, &capacity, (uint16_t)value))
    {
      free(units);
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while parsing XLIL const.str");
    }
    cursor += 6;
    while(cursor < end && (*cursor == ' ' || *cursor == '\t'))
      ++cursor;
    if(cursor == end)
      break;
    if(*cursor != ',')
      goto invalid;
    ++cursor;
    if(cursor >= end || *cursor != ' ')
      goto invalid;
    while(cursor < end && *cursor == ' ')
      ++cursor;
    if(cursor == end)
      goto invalid;
  }
  XsLilValueId actual = 0;
  XsLilStatus status = xs_lil_block_add_const_str(block, encoding, units, length, &actual, error);
  free(units);
  if(status != XS_LIL_OK)
    return status;
  return actual == expected_result
             ? XS_LIL_OK
             : xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL value ids must be sequential");
invalid:
  free(units);
  return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL const.str UTF-16 code units are invalid");
}
