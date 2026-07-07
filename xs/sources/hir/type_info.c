/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/hir/type_info.h"

#include <string.h>

static const XsHirPrimitiveInfo primitive_types[] = {
    {.name = "str", .kind = XS_HIR_PRIMITIVE_STR, .code_unit_bit_width = 16, .is_text = true},
    {.name = "bool",
     .kind = XS_HIR_PRIMITIVE_BOOL,
     .bit_width = 1,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_BOOL}},
    {.name = "byte",
     .kind = XS_HIR_PRIMITIVE_BYTE,
     .bit_width = 8,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_U8}},
    {.name = "sbyte",
     .kind = XS_HIR_PRIMITIVE_SBYTE,
     .bit_width = 8,
     .is_signed = true,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_I8}},
    {.name = "char",
     .kind = XS_HIR_PRIMITIVE_CHAR,
     .bit_width = 16,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_U16}},
    {.name = "int16",
     .kind = XS_HIR_PRIMITIVE_INT16,
     .bit_width = 16,
     .is_signed = true,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_I16}},
    {.name = "int32",
     .kind = XS_HIR_PRIMITIVE_INT32,
     .bit_width = 32,
     .is_signed = true,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_I32}},
    {.name = "int",
     .kind = XS_HIR_PRIMITIVE_INT,
     .bit_width = 64,
     .is_signed = true,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_I64}},
    {.name = "int128",
     .kind = XS_HIR_PRIMITIVE_INT128,
     .bit_width = 128,
     .is_signed = true,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_I128}},
    {.name = "Uint16",
     .kind = XS_HIR_PRIMITIVE_UINT16,
     .bit_width = 16,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_U16}},
    {.name = "Uint32",
     .kind = XS_HIR_PRIMITIVE_UINT32,
     .bit_width = 32,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_U32}},
    {.name = "Uint",
     .kind = XS_HIR_PRIMITIVE_UINT,
     .bit_width = 64,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_U64}},
    {.name = "Uint128",
     .kind = XS_HIR_PRIMITIVE_UINT128,
     .bit_width = 128,
     .is_integer = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_U128}},
    {.name = "float32",
     .kind = XS_HIR_PRIMITIVE_FLOAT32,
     .bit_width = 32,
     .is_float = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_F32}},
    {.name = "float",
     .kind = XS_HIR_PRIMITIVE_FLOAT,
     .bit_width = 64,
     .is_float = true,
     .has_xlil_type = true,
     .xlil_type = {.kind = XS_LIL_TYPE_F64}},
};

const XsHirPrimitiveInfo *xs_hir_primitive_find(const char *name, size_t length)
{
  if (name == nullptr)
    return nullptr;
  for (size_t i = 0; i < sizeof(primitive_types) / sizeof(primitive_types[0]); ++i)
  {
    if (strlen(primitive_types[i].name) == length && memcmp(primitive_types[i].name, name, length) == 0)
      return &primitive_types[i];
  }
  return nullptr;
}
