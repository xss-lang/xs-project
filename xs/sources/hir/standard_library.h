/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_HIR_STANDARD_LIBRARY_H
#define XS_HIR_STANDARD_LIBRARY_H

#include "xs/hir/symbol_table.h"

typedef struct
{
  const char *canonical_name;
  const char *short_name;
  const char *required_import;
  const char *alternate_import;
  size_t min_arity;
  size_t max_arity;
} XsHirStandardTypeInfo;

typedef enum
{
  XS_HIR_STANDARD_UNKNOWN,
  XS_HIR_STANDARD_AVAILABLE,
  XS_HIR_STANDARD_MISSING_IMPORT,
} XsHirStandardLookup;

const XsHirStandardTypeInfo *xs_hir_standard_type_find(const char *name);
XsHirStandardLookup xs_hir_standard_type_lookup(const XsHirStandardTypeInfo *type, const XsHirImportScope *imports);
XsHirStandardLookup xs_hir_standard_call_lookup(const char *name, const XsHirImportScope *imports);

#endif
