/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_DRIVER_SOURCE_NATIVE_INTERNAL_H
#define XS_DRIVER_SOURCE_NATIVE_INTERNAL_H

#include "xs/diagnostic.h"
#include "xs/lil.h"
#include "xs/mir.h"
#include "xs/syntax_ast.h"

#include <stddef.h>
#include <stdint.h>

typedef struct
{
  XsText name;
  XsMirValueId value;
  XsLilTypeKind type;
} NativeBinding;

typedef struct
{
  NativeBinding items[32];
  size_t count;
} NativeContext;

XsSpan xs_source_native_node_span(const XsSyntaxNode *node);
bool xs_source_native_text_equals(XsText left, XsText right);
bool xs_source_native_node_text_equals(const XsSyntaxNode *node, const char *text);
char *xs_source_native_copy_node_text(const XsSyntaxNode *node);
char *xs_source_native_copy_cstr(const char *text);
void xs_source_native_set_mir_error(XsMirError *error, XsMirStatus status, const char *message);
void xs_source_native_context_init_parameters(NativeContext *context, const XsSyntaxNode *function);
bool xs_source_native_context_add(NativeContext *context, XsText name, XsMirValueId value, XsLilTypeKind type,
                                  XsDiagnostics *diagnostics, XsSpan span);
bool xs_source_native_context_find(const NativeContext *context, XsText name, XsLilTypeKind type, XsMirValueId *value);
const XsSyntaxNode *xs_source_native_first_child_kind(const XsSyntaxNode *node, XsSyntaxKind kind);
size_t xs_source_native_child_count_kind(const XsSyntaxNode *node, XsSyntaxKind kind);
bool xs_source_native_parse_i32_literal(const XsSyntaxNode *literal, int32_t *value);

#endif
