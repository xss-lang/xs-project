/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_HIR_INHERITANCE_H
#define XS_HIR_INHERITANCE_H

#include "xs/hir/symbol_table.h"

bool xs_hir_validate_inheritance(const XsSyntaxTree *tree, const XsHirSymbolTable *symbols,
                                 const XsHirImportScope *import, XsDiagnostics *diagnostics);

#endif
