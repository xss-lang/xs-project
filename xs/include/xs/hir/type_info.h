/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_HIR_TYPE_INFO_H
#define XS_HIR_TYPE_INFO_H

#include "xs/lil.h"

#include <stddef.h>

typedef enum
{
  XS_HIR_PRIMITIVE_STR,
  XS_HIR_PRIMITIVE_BOOL,
  XS_HIR_PRIMITIVE_BYTE,
  XS_HIR_PRIMITIVE_SBYTE,
  XS_HIR_PRIMITIVE_CHAR,
  XS_HIR_PRIMITIVE_INT16,
  XS_HIR_PRIMITIVE_INT32,
  XS_HIR_PRIMITIVE_INT,
  XS_HIR_PRIMITIVE_INT128,
  XS_HIR_PRIMITIVE_UINT16,
  XS_HIR_PRIMITIVE_UINT32,
  XS_HIR_PRIMITIVE_UINT,
  XS_HIR_PRIMITIVE_UINT128,
  XS_HIR_PRIMITIVE_FLOAT32,
  XS_HIR_PRIMITIVE_FLOAT,
} XsHirPrimitiveKind;

typedef struct
{
  const char *name;
  size_t bit_width;
  size_t code_unit_bit_width;
  XsHirPrimitiveKind kind;
  bool is_signed;
  bool is_integer;
  bool is_float;
  bool is_text;
  bool has_xlil_type;
  XsLilType xlil_type;
} XsHirPrimitiveInfo;

const XsHirPrimitiveInfo *xs_hir_primitive_find(const char *name, size_t length);

#endif
