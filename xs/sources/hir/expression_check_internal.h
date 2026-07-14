/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_HIR_EXPRESSION_CHECK_INTERNAL_H
#define XS_HIR_EXPRESSION_CHECK_INTERNAL_H

#include "xs/hir/expression_check.h"

bool xs_hir_check_result_constructor_call(const XsSyntaxNode *node, bool enclosing_returns_result,
                                          XsDiagnostics *diagnostics);
bool xs_hir_type_returns_result(const XsSyntaxNode *type);
bool xs_hir_type_is_string_sugar(const XsSyntaxNode *type);
bool xs_hir_check_string_sugar_value(const XsSyntaxNode *expression, XsDiagnostics *diagnostics);

#endif
