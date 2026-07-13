/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "source_native_internal.h"

#include <stdint.h>

static bool static_bool_condition(const XsSyntaxNode *expression, bool *value)
{
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL &&
     (expression->token_kind == XS_TOKEN_KW_TRUE || expression->token_kind == XS_TOKEN_KW_FALSE))
  {
    *value = expression->token_kind == XS_TOKEN_KW_TRUE;
    return true;
  }
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_BANG &&
     expression->child_count == 1 && static_bool_condition(expression->children[0], value))
  {
    *value = !*value;
    return true;
  }
  return false;
}

static bool static_i32_expression(const XsSyntaxNode *expression, int32_t *value)
{
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_MINUS &&
     expression->child_count == 1)
  {
    int32_t nested = 0;
    if(!static_i32_expression(expression->children[0], &nested) || nested == INT32_MIN)
      return false;
    *value = -nested;
    return true;
  }
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_PLUS &&
     expression->child_count == 1)
    return static_i32_expression(expression->children[0], value);
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL && expression->token_kind == XS_TOKEN_INTEGER)
    return xs_source_native_parse_i32_literal(expression, value);
  if(expression->kind != XS_SYNTAX_EXPR_BINARY || expression->child_count != 3)
    return false;
  int32_t left = 0;
  int32_t right = 0;
  if(!static_i32_expression(expression->children[0], &left) || !static_i32_expression(expression->children[2], &right))
    return false;
  if((expression->token_kind == XS_TOKEN_SLASH || expression->token_kind == XS_TOKEN_PERCENT) &&
     (right == 0 || (left == INT32_MIN && right == -1)))
    return false;
  if((expression->token_kind == XS_TOKEN_SHIFT_LEFT || expression->token_kind == XS_TOKEN_SHIFT_RIGHT) &&
     (right < 0 || right >= 32))
    return false;
  if(expression->token_kind == XS_TOKEN_SHIFT_LEFT && (left < 0 || left > (INT32_MAX >> (unsigned)right)))
    return false;
  if(expression->token_kind == XS_TOKEN_SHIFT_RIGHT && left < 0)
    return false;
  int64_t result = expression->token_kind == XS_TOKEN_PLUS          ? (int64_t)left + right
                   : expression->token_kind == XS_TOKEN_MINUS       ? (int64_t)left - right
                   : expression->token_kind == XS_TOKEN_STAR        ? (int64_t)left * right
                   : expression->token_kind == XS_TOKEN_SLASH       ? left / right
                   : expression->token_kind == XS_TOKEN_PERCENT     ? left % right
                   : expression->token_kind == XS_TOKEN_AMPERSAND   ? left & right
                   : expression->token_kind == XS_TOKEN_PIPE        ? left | right
                   : expression->token_kind == XS_TOKEN_CARET       ? left ^ right
                   : expression->token_kind == XS_TOKEN_SHIFT_LEFT  ? left << (unsigned)right
                   : expression->token_kind == XS_TOKEN_SHIFT_RIGHT ? left >> (unsigned)right
                                                                    : (int64_t)INT32_MIN - 1;
  if(result < INT32_MIN || result > INT32_MAX)
    return false;
  *value = (int32_t)result;
  return true;
}

static bool static_i32_comparison(const XsSyntaxNode *expression, bool *value)
{
  if(expression->kind != XS_SYNTAX_EXPR_BINARY || expression->child_count != 3)
    return false;
  int32_t left = 0;
  int32_t right = 0;
  if(!static_i32_expression(expression->children[0], &left) || !static_i32_expression(expression->children[2], &right))
    return false;
  switch(expression->token_kind)
  {
  case XS_TOKEN_EQUAL:
    *value = left == right;
    return true;
  case XS_TOKEN_NOT_EQUAL:
    *value = left != right;
    return true;
  case XS_TOKEN_LESS:
    *value = left < right;
    return true;
  case XS_TOKEN_LESS_EQUAL:
    *value = left <= right;
    return true;
  case XS_TOKEN_GREATER:
    *value = left > right;
    return true;
  case XS_TOKEN_GREATER_EQUAL:
    *value = left >= right;
    return true;
  default:
    return false;
  }
}

bool xs_source_native_static_condition(const XsSyntaxNode *expression, bool *value)
{
  if(static_bool_condition(expression, value) || static_i32_comparison(expression, value))
    return true;
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_BANG &&
     expression->child_count == 1 && xs_source_native_static_condition(expression->children[0], value))
  {
    *value = !*value;
    return true;
  }
  return false;
}
