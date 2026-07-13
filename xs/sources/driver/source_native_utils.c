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
  context->scope_depth = 1;
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

bool xs_source_native_context_enter_scope(NativeContext *context, XsMirError *error)
{
  if(context->scope_depth == sizeof(context->scope_starts) / sizeof(context->scope_starts[0]))
  {
    xs_source_native_set_mir_error(error, XS_MIR_UNSUPPORTED, "native source lexical scope nesting exceeds 32 levels");
    return false;
  }
  context->scope_starts[context->scope_depth++] = context->count;
  return true;
}

void xs_source_native_context_exit_scope(NativeContext *context)
{
  if(context->scope_depth <= 1)
    return;
  context->count = context->scope_starts[--context->scope_depth];
}

static const NativeBinding *context_find_binding(const NativeContext *context, XsText name)
{
  for(size_t i = context->count; i > 0; --i)
  {
    if(xs_source_native_text_equals(context->items[i - 1].name, name))
      return &context->items[i - 1];
  }
  return nullptr;
}

static bool context_current_scope_has_binding(const NativeContext *context, XsText name)
{
  size_t start = context->scope_depth == 0 ? 0 : context->scope_starts[context->scope_depth - 1];
  for(size_t index = context->count; index > start; --index)
  {
    if(xs_source_native_text_equals(context->items[index - 1].name, name))
      return true;
  }
  return false;
}

bool xs_source_native_context_add_local(NativeContext *context, XsMirFunction *function, XsMirBlock *block, XsText name,
                                        XsMirValueId value, XsLilTypeKind type, bool is_mutable,
                                        XsDiagnostics *diagnostics, XsSpan span, XsMirError *error)
{
  if(context_current_scope_has_binding(context, name))
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "native source local names must be unique inside one lexical scope") &&
           false;
  if(context->count == sizeof(context->items) / sizeof(context->items[0]))
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "native source main supports at most 32 local bindings for now") &&
           false;
  char *local_name = malloc(name.length + 1U);
  if(local_name == nullptr)
  {
    xs_source_native_set_mir_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while naming native source local");
    return false;
  }
  memcpy(local_name, name.data, name.length);
  local_name[name.length] = '\0';
  XsMirLocalId local = 0;
  XsMirPlace *place = nullptr;
  XsMirType mir_type = {.kind = type};
  XsMirStatus status =
      xs_mir_function_add_local(function, XS_MIR_LOCAL_VARIABLE, local_name, mir_type, is_mutable, &local, error);
  free(local_name);
  if(status != XS_MIR_OK || xs_mir_function_add_local_place(function, local, &place, error) != XS_MIR_OK ||
     xs_mir_block_add_store(block, place, value, error) != XS_MIR_OK)
    return false;
  context->items[context->count++] =
      (NativeBinding){.name = name, .value = value, .place = place, .type = type, .is_mutable = is_mutable};
  return true;
}

bool xs_source_native_context_read(const NativeContext *context, XsMirBlock *block, XsText name, XsLilTypeKind type,
                                   XsMirValueId *value, XsMirError *error)
{
  const NativeBinding *binding = context_find_binding(context, name);
  if(binding == nullptr || binding->type != type)
    return false;
  if(binding->place == nullptr)
  {
    *value = binding->value;
    return true;
  }
  return xs_mir_block_add_load(block, binding->place, (XsMirType){.kind = type}, value, error) == XS_MIR_OK;
}

bool xs_source_native_context_type(const NativeContext *context, XsText name, XsLilTypeKind *type)
{
  const NativeBinding *binding = context_find_binding(context, name);
  if(binding == nullptr)
    return false;
  *type = binding->type;
  return true;
}

bool xs_source_native_context_assign(const NativeContext *context, XsMirBlock *block, XsText name, XsLilTypeKind type,
                                     XsMirValueId value, XsDiagnostics *diagnostics, XsSpan span, XsMirError *error)
{
  const NativeBinding *binding = context_find_binding(context, name);
  if(binding == nullptr)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "native source assignment target is unknown in this compiler slice") &&
           false;
  if(binding->type != type)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "native source assignment value does not match the local type") &&
           false;
  if(binding->place == nullptr || !binding->is_mutable)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                              "native source assignment target is not mutable") &&
           false;
  return xs_mir_block_add_store(block, binding->place, value, error) == XS_MIR_OK;
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
