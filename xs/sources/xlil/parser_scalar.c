/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

XsLilStatus xs_lil_parse_const_u16(XsLilBlock *block, XsLilType result_type, const char *operation,
                                   size_t operation_length, XsLilValueId expected_result, bool *matched,
                                   XsLilError *error)
{
  static const char prefix[] = "const.u16 0x";
  *matched = operation_length >= sizeof(prefix) - 1U && memcmp(operation, prefix, sizeof(prefix) - 1U) == 0;
  if(!*matched)
    return XS_LIL_OK;
  if(result_type.kind != XS_LIL_TYPE_U16 || operation_length != sizeof(prefix) - 1U + 4U)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL const.u16 record is invalid");
  char digits[5] = {0};
  memcpy(digits, operation + sizeof(prefix) - 1U, 4);
  char *tail = nullptr;
  errno = 0;
  unsigned long value = strtoul(digits, &tail, 16);
  if(errno != 0 || tail == digits || *tail != '\0')
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL const.u16 immediate is invalid");
  XsLilValueId actual = 0;
  XsLilStatus status = xs_lil_block_add_const_u16(block, (uint16_t)value, &actual, error);
  if(status != XS_LIL_OK)
    return status;
  return actual == expected_result
             ? XS_LIL_OK
             : xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL value ids must be sequential");
}
