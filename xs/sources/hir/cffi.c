/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/hir/cffi.h"
#include "syntax_helpers.h"

#include <stdlib.h>
#include <string.h>

static bool text_equals(XsText text, const char *expected)
{
  size_t length = strlen(expected);
  return text.length == length && memcmp(text.data, expected, length) == 0;
}

static bool path_text_equals(const XsSyntaxNode *path, const char *expected)
{
  char *text = xs_hir_path_to_string(path);
  bool same = text != nullptr && strcmp(text, expected) == 0;
  free(text);
  return same;
}

static bool error_at(XsDiagnostics *diagnostics, const XsSyntaxNode *node, const char *message)
{
  xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(node), message);
  return false;
}

static bool attribute_argument_is_c(const XsSyntaxNode *attribute)
{
  size_t argument_count = 0;
  const XsSyntaxNode *argument = nullptr;
  for(size_t i = 0; i < attribute->child_count; ++i)
  {
    if(attribute->children[i]->kind == XS_SYNTAX_PATH)
      continue;
    ++argument_count;
    argument = attribute->children[i];
  }
  return argument_count == 1 && argument != nullptr && argument->kind == XS_SYNTAX_EXPR_IDENTIFIER &&
         text_equals(argument->text, "C");
}

static bool validate_repr_attribute(const XsSyntaxNode *attribute, XsDiagnostics *diagnostics)
{
  if(attribute_argument_is_c(attribute))
    return true;
  return error_at(diagnostics, attribute, "C ABI extern blocks require #[repr(C)]");
}

static bool has_valid_repr_c(const XsSyntaxNode *node, XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *attributes = xs_hir_first_child_kind(node, XS_SYNTAX_ATTRIBUTE_LIST);
  if(attributes == nullptr)
    return error_at(diagnostics, node, "C ABI extern blocks require #[repr(C)]");
  for(size_t i = 0; i < attributes->child_count; ++i)
  {
    const XsSyntaxNode *attribute = attributes->children[i];
    const XsSyntaxNode *path = xs_hir_first_child_kind(attribute, XS_SYNTAX_PATH);
    if(path_text_equals(path, "repr"))
      return validate_repr_attribute(attribute, diagnostics);
  }
  return error_at(diagnostics, node, "C ABI extern blocks require #[repr(C)]");
}

static bool validate_extern_block(const XsSyntaxNode *node, XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *abi = xs_hir_first_child_kind(node, XS_SYNTAX_TOKEN);
  if(abi == nullptr || !text_equals(abi->text, "\"C\""))
    return error_at(diagnostics, node, "only extern \"C\" blocks are supported");
  return has_valid_repr_c(node, diagnostics);
}

static bool validate_node(const XsSyntaxNode *node, XsDiagnostics *diagnostics)
{
  bool success = true;
  if(node->kind == XS_SYNTAX_DECL_EXTERN_BLOCK)
    success = validate_extern_block(node, diagnostics);
  for(size_t i = 0; i < node->child_count; ++i)
    success = validate_node(node->children[i], diagnostics) && success;
  return success;
}

bool xs_hir_validate_cffi(const XsSyntaxTree *tree, XsDiagnostics *diagnostics)
{
  if(tree == nullptr || tree->root == nullptr)
    return true;
  return validate_node(tree->root, diagnostics);
}
