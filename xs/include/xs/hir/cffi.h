/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_HIR_CFFI_H
#define XS_HIR_CFFI_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

bool xs_hir_validate_cffi(const XsSyntaxTree *tree, XsDiagnostics *diagnostics);

#endif
