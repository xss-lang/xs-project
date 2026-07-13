/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "expression_check_internal.h"

#include <string.h>

static bool text_matches(XsText text, const char *value)
{
  size_t length = strlen(value);
  return text.length == length && memcmp(text.data, value, length) == 0;
}

bool xs_hir_check_result_constructor_call(const XsSyntaxNode *node, bool enclosing_returns_result,
                                          XsDiagnostics *diagnostics)
{
  if(node == nullptr || node->kind != XS_SYNTAX_EXPR_CALL || node->child_count == 0)
    return true;
  const XsSyntaxNode *callee = node->children[0];
  if(callee->kind != XS_SYNTAX_EXPR_IDENTIFIER || callee->child_count != 1 ||
     callee->children[0]->kind != XS_SYNTAX_IDENTIFIER)
    return true;
  XsText name = callee->children[0]->text;
  if(!text_matches(name, "Ok") && !text_matches(name, "Error"))
    return true;
  if(enclosing_returns_result)
    return true;
  XsSpan span = {.start = node->span.start_offset, .end = node->span.end_offset};
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                            "Ok and Error constructors require an enclosing function returning Result<T, E>");
}
