/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "source_native_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

XsSpan xs_source_native_node_span(const XsSyntaxNode *node)
{
  return (XsSpan){.start = node->span.start_offset, .end = node->span.end_offset};
}

bool xs_source_native_text_equals(XsText left, XsText right)
{
  return left.length == right.length && memcmp(left.data, right.data, left.length) == 0;
}

bool xs_source_native_node_text_equals(const XsSyntaxNode *node, const char *text)
{
  size_t length = strlen(text);
  return node != nullptr && node->text.length == length && memcmp(node->text.data, text, length) == 0;
}

char *xs_source_native_copy_node_text(const XsSyntaxNode *node)
{
  if(node == nullptr)
    return nullptr;
  char *result = malloc(node->text.length + 1U);
  if(result == nullptr)
    return nullptr;
  memcpy(result, node->text.data, node->text.length);
  result[node->text.length] = '\0';
  return result;
}

char *xs_source_native_copy_cstr(const char *text)
{
  size_t length = strlen(text);
  char *result = malloc(length + 1U);
  if(result != nullptr)
    memcpy(result, text, length + 1U);
  return result;
}

void xs_source_native_set_mir_error(XsMirError *error, XsMirStatus status, const char *message)
{
  if(error != nullptr)
  {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == nullptr ? "source native MIR error" : message);
  }
}

void xs_source_native_context_init_parameters(NativeContext *context, const XsSyntaxNode *function)
{
  *context = (NativeContext){0};
  XsMirValueId value = 0;
  for(size_t i = 0; i < function->child_count; ++i)
  {
    const XsSyntaxNode *parameter = function->children[i];
    if(parameter->kind != XS_SYNTAX_PARAMETER || parameter->child_count < 2)
      continue;
    context->items[context->count++] =
        (NativeBinding){.name = parameter->children[0]->text, .value = value++, .type = XS_LIL_TYPE_I32};
  }
}

bool xs_source_native_context_add(NativeContext *context, XsText name, XsMirValueId value, XsLilTypeKind type,
                                  XsDiagnostics *diagnostics, XsSpan span)
{
  for(size_t i = 0; i < context->count; ++i)
  {
    if(xs_source_native_text_equals(context->items[i].name, name))
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                                "native source local names must be unique in this compiler slice") &&
             false;
  }
  if(context->count == sizeof(context->items) / sizeof(context->items[0]))
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "native source main supports at most 32 local bindings for now") &&
           false;
  context->items[context->count++] = (NativeBinding){.name = name, .value = value, .type = type};
  return true;
}

bool xs_source_native_context_find(const NativeContext *context, XsText name, XsLilTypeKind type, XsMirValueId *value)
{
  for(size_t i = context->count; i > 0; --i)
  {
    if(xs_source_native_text_equals(context->items[i - 1].name, name) && context->items[i - 1].type == type)
    {
      *value = context->items[i - 1].value;
      return true;
    }
  }
  return false;
}

const XsSyntaxNode *xs_source_native_first_child_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if(node == nullptr)
    return nullptr;
  for(size_t i = 0; i < node->child_count; ++i)
  {
    if(node->children[i]->kind == kind)
      return node->children[i];
  }
  return nullptr;
}

size_t xs_source_native_child_count_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  size_t count = 0;
  if(node == nullptr)
    return 0;
  for(size_t i = 0; i < node->child_count; ++i)
  {
    if(node->children[i]->kind == kind)
      ++count;
  }
  return count;
}

bool xs_source_native_parse_i32_literal(const XsSyntaxNode *literal, int32_t *value)
{
  int64_t result = 0;
  bool saw_digit = false;
  for(size_t i = 0; i < literal->text.length; ++i)
  {
    char c = literal->text.data[i];
    if(c == '\'')
      continue;
    if(c < '0' || c > '9')
      return false;
    saw_digit = true;
    int64_t digit = c - '0';
    if(result > (INT32_MAX - digit) / 10)
      return false;
    result = result * 10 + digit;
  }
  if(!saw_digit)
    return false;
  *value = (int32_t)result;
  return true;
}
