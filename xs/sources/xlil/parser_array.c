/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <stdlib.h>
#include <string.h>

XsLilStatus xs_lil_parse_array_record(XsLilModule *module, const char *text, size_t length, XsLilError *error)
{
  const char *end = text + length;
  const char *cursor = text + strlen(".array %a");
  char *id_end = nullptr;
  unsigned long id = strtoul(cursor, &id_end, 10);
  if(id_end == cursor || id > UINT32_MAX || id != xs_lil_module_array_type_count(module) || end - id_end < 4 ||
     strncmp(id_end, " : ", 3) != 0)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL array type id is invalid or non-sequential");
  cursor = id_end + 3;
  const char *separator = strstr(cursor, " x ");
  XsLilType element_type = {0};
  const char *element_end = separator == nullptr ? end : separator;
  if(element_end == cursor || !xs_lil_parse_type_name(module, cursor, (size_t)(element_end - cursor), &element_type))
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL array element type is invalid");
  XsLilType result = {0};
  if(separator == nullptr)
    return xs_lil_module_add_dynamic_array_type(module, element_type, &result, error);
  if(separator + 3 >= end)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL array type record is invalid");
  char *length_end = nullptr;
  unsigned long long array_length = strtoull(separator + 3, &length_end, 10);
  if(length_end != end || array_length == 0)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL array length is invalid");
  return xs_lil_module_add_array_type(module, element_type, (uint64_t)array_length, &result, error);
}
