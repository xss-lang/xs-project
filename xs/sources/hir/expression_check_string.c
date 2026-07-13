/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "expression_check_internal.h"

#include <string.h>

static bool text_is(XsText text, const char *expected)
{
  size_t length = strlen(expected);
  return text.length == length && memcmp(text.data, expected, length) == 0;
}

static const XsSyntaxNode *first_child(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if(node == nullptr)
    return nullptr;
  for(size_t index = 0; index < node->child_count; ++index)
  {
    if(node->children[index]->kind == kind)
      return node->children[index];
  }
  return nullptr;
}

static bool path_is_single(const XsSyntaxNode *path, const char *name)
{
  return path != nullptr && path->kind == XS_SYNTAX_PATH && path->child_count == 1 &&
         path->children[0]->kind == XS_SYNTAX_IDENTIFIER && text_is(path->children[0]->text, name);
}

bool xs_hir_type_is_string_sugar(const XsSyntaxNode *type)
{
  return type != nullptr && type->kind == XS_SYNTAX_TYPE_NAMED &&
         path_is_single(first_child(type, XS_SYNTAX_PATH), "String");
}

static bool expression_names(const XsSyntaxNode *expression, const char *name)
{
  if(expression == nullptr || expression->kind != XS_SYNTAX_EXPR_IDENTIFIER || expression->child_count != 1)
    return false;
  const XsSyntaxNode *name_node = expression->children[0];
  if(name_node->kind == XS_SYNTAX_IDENTIFIER)
    return text_is(name_node->text, name);
  if(name_node->kind != XS_SYNTAX_PATH || name_node->child_count == 0)
    return false;
  const XsSyntaxNode *last = name_node->children[name_node->child_count - 1];
  return last->kind == XS_SYNTAX_IDENTIFIER && text_is(last->text, name);
}

static bool is_none(const XsSyntaxNode *expression)
{
  return expression != nullptr && expression->kind == XS_SYNTAX_EXPR_LITERAL &&
         expression->token_kind == XS_TOKEN_KW_NONE;
}

static bool report_invalid_string_value(const XsSyntaxNode *expression, XsDiagnostics *diagnostics)
{
  XsSpan span = {.start = expression->span.start_offset, .end = expression->span.end_offset};
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                            "String sugar accepts only a Str value, Some(Str), or None");
}

bool xs_hir_check_string_sugar_value(const XsSyntaxNode *expression, XsDiagnostics *diagnostics)
{
  if(expression == nullptr || is_none(expression))
    return true;
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL)
    return expression->token_kind == XS_TOKEN_STRING || report_invalid_string_value(expression, diagnostics);
  if(expression->kind != XS_SYNTAX_EXPR_CALL || expression->child_count == 0 ||
     !expression_names(expression->children[0], "Some"))
    return true;
  if(expression->child_count != 2)
    return report_invalid_string_value(expression, diagnostics);
  const XsSyntaxNode *payload = expression->children[1];
  if(payload->kind == XS_SYNTAX_EXPR_LITERAL && payload->token_kind != XS_TOKEN_STRING)
    return report_invalid_string_value(payload, diagnostics);
  return true;
}
