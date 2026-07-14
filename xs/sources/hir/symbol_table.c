/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/hir/symbol_table.h"

#include "syntax_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const XsSyntaxNode *declaration_name_node(const XsSyntaxNode *declaration)
{
  return xs_hir_first_child_kind(declaration, XS_SYNTAX_IDENTIFIER);
}

static bool symbol_kind_for_node(XsSyntaxKind syntax, XsHirSymbolKind *kind)
{
  switch(syntax)
  {
  case XS_SYNTAX_DECL_FUNCTION:
    *kind = XS_HIR_SYMBOL_FUNCTION;
    return true;
  case XS_SYNTAX_DECL_CLASS:
    *kind = XS_HIR_SYMBOL_CLASS;
    return true;
  case XS_SYNTAX_DECL_INTERFACE:
    *kind = XS_HIR_SYMBOL_INTERFACE;
    return true;
  case XS_SYNTAX_DECL_ENUM:
    *kind = XS_HIR_SYMBOL_ENUM;
    return true;
  case XS_SYNTAX_DECL_DATA:
    *kind = XS_HIR_SYMBOL_DATA;
    return true;
  case XS_SYNTAX_DECL_MACRO:
    *kind = XS_HIR_SYMBOL_MACRO;
    return true;
  default:
    return false;
  }
}

static bool symbol_kind_for_declaration(const XsSyntaxNode *node, XsHirSymbolKind *kind)
{
  if(node != nullptr && node->kind == XS_SYNTAX_DECL_VARIABLE && (node->flags & XS_SYNTAX_FLAG_EXTERN) != 0)
  {
    *kind = XS_HIR_SYMBOL_EXTERN_GLOBAL;
    return true;
  }
  return node != nullptr && symbol_kind_for_node(node->kind, kind);
}

static bool append_symbol(XsHirSymbolTable *table, XsHirSymbol symbol)
{
  if(table->count == table->capacity)
  {
    size_t capacity = table->capacity == 0 ? 16 : table->capacity * 2;
    XsHirSymbol *symbols = realloc(table->symbols, capacity * sizeof(*symbols));
    if(symbols == nullptr)
    {
      table->allocation_failed = true;
      return false;
    }
    table->symbols = symbols;
    table->capacity = capacity;
  }
  table->symbols[table->count++] = symbol;
  return true;
}

static bool append_member_symbol(XsHirMemberSymbolTable *table, XsHirMemberSymbol symbol)
{
  if(table->count == table->capacity)
  {
    size_t capacity = table->capacity == 0 ? 16 : table->capacity * 2;
    XsHirMemberSymbol *members = realloc(table->members, capacity * sizeof(*members));
    if(members == nullptr)
    {
      table->allocation_failed = true;
      return false;
    }
    table->members = members;
    table->capacity = capacity;
  }
  table->members[table->count++] = symbol;
  return true;
}

static const XsHirSymbol *find_in_namespace(const XsHirSymbolTable *table, const char *namespace_name, const char *name)
{
  for(size_t i = 0; i < table->count; ++i)
  {
    if(strcmp(table->symbols[i].namespace_name, namespace_name) == 0 && strcmp(table->symbols[i].name, name) == 0)
      return &table->symbols[i];
  }
  return nullptr;
}

static const XsHirMemberSymbol *find_member_forward(const XsHirMemberSymbolTable *table, const char *owner,
                                                    const char *name)
{
  for(size_t i = 0; i < table->count; ++i)
  {
    if(strcmp(table->members[i].owner_qualified_name, owner) == 0 && strcmp(table->members[i].name, name) == 0)
      return &table->members[i];
  }
  return nullptr;
}

static bool report_duplicate(XsDiagnostics *diagnostics, const XsSyntaxNode *node, const XsHirSymbol *previous)
{
  char message[512];
  snprintf(message, sizeof(message), "symbol conflicts with previous %s declaration '%s'",
           xs_hir_symbol_kind_name(previous->kind), previous->qualified_name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(node), message);
}

static bool report_duplicate_member(XsDiagnostics *diagnostics, const XsSyntaxNode *node,
                                    const XsHirMemberSymbol *previous)
{
  char message[512];
  snprintf(message, sizeof(message), "member conflicts with previous %s declaration '%s'",
           xs_hir_member_kind_name(previous->kind), previous->qualified_name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(node), message);
}

static bool tree_has_public_namespace(const XsSyntaxTree *tree)
{
  for(size_t i = 0; i < tree->root->child_count; ++i)
  {
    const XsSyntaxNode *child = tree->root->children[i];
    if(child->kind == XS_SYNTAX_DECL_NAMESPACE && child->visibility == XS_SYNTAX_VISIBILITY_PUBLIC)
      return true;
  }
  return false;
}

static XsSyntaxVisibility declaration_visibility(const XsSyntaxNode *node, bool public_namespace)
{
  if(node->visibility != XS_SYNTAX_VISIBILITY_DEFAULT)
    return node->visibility;
  (void)public_namespace;
  return XS_SYNTAX_VISIBILITY_INTERNAL;
}

static XsSyntaxVisibility member_visibility(const XsSyntaxNode *node)
{
  return node->visibility == XS_SYNTAX_VISIBILITY_DEFAULT ? XS_SYNTAX_VISIBILITY_INTERNAL : node->visibility;
}

static bool collect_declaration_with_visibility(const XsSyntaxNode *node, const char *namespace_name,
                                                XsSyntaxVisibility visibility, XsHirSymbolTable *table,
                                                XsDiagnostics *diagnostics)
{
  XsHirSymbolKind kind;
  if(!symbol_kind_for_declaration(node, &kind))
    return true;
  const XsSyntaxNode *name_node = declaration_name_node(node);
  if(name_node == nullptr)
    return true;
  char *name = xs_hir_copy_text(name_node->text);
  char *namespace_copy = xs_hir_copy_cstr(namespace_name);
  if(name == nullptr || namespace_copy == nullptr)
  {
    free(name);
    free(namespace_copy);
    table->allocation_failed = true;
    return false;
  }
  const XsHirSymbol *previous = find_in_namespace(table, namespace_name, name);
  if(previous != nullptr)
  {
    report_duplicate(diagnostics, name_node, previous);
    free(name);
    free(namespace_copy);
    return true;
  }
  char *qualified = xs_hir_join_qualified(namespace_name, name);
  if(qualified == nullptr)
  {
    free(name);
    free(namespace_copy);
    table->allocation_failed = true;
    return false;
  }
  XsHirSymbol symbol = {
      .kind = kind,
      .name = name,
      .namespace_name = namespace_copy,
      .qualified_name = qualified,
      .visibility = visibility,
      .span = name_node->span,
      .syntax = node,
  };
  if(!append_symbol(table, symbol))
  {
    free(name);
    free(namespace_copy);
    free(qualified);
    return false;
  }
  return true;
}

static bool collect_declaration(const XsSyntaxNode *node, const char *namespace_name, bool public_namespace,
                                XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  return collect_declaration_with_visibility(node, namespace_name, declaration_visibility(node, public_namespace),
                                             table, diagnostics);
}

static bool collect_extern_block(const XsSyntaxNode *node, const char *namespace_name, bool public_namespace,
                                 XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  XsSyntaxVisibility block_visibility = declaration_visibility(node, public_namespace);
  for(size_t i = 0; i < node->child_count; ++i)
  {
    const XsSyntaxNode *child = node->children[i];
    if(child->kind != XS_SYNTAX_DECL_FUNCTION && child->kind != XS_SYNTAX_DECL_VARIABLE)
      continue;
    XsSyntaxVisibility child_visibility = child->visibility == XS_SYNTAX_VISIBILITY_DEFAULT
                                              ? block_visibility
                                              : declaration_visibility(child, public_namespace);
    if(!collect_declaration_with_visibility(child, namespace_name, child_visibility, table, diagnostics))
      return false;
  }
  return true;
}

static bool member_kind_for_node(XsSyntaxKind syntax, XsHirMemberKind *kind)
{
  switch(syntax)
  {
  case XS_SYNTAX_CLASS_FIELD:
  case XS_SYNTAX_DATA_FIELD:
  case XS_SYNTAX_DECL_VARIABLE:
    *kind = XS_HIR_MEMBER_FIELD;
    return true;
  case XS_SYNTAX_DECL_FUNCTION:
    *kind = XS_HIR_MEMBER_METHOD;
    return true;
  case XS_SYNTAX_CLASS_CONSTRUCTOR:
    *kind = XS_HIR_MEMBER_CONSTRUCTOR;
    return true;
  case XS_SYNTAX_CLASS_DESTRUCTOR:
    *kind = XS_HIR_MEMBER_DESTRUCTOR;
    return true;
  case XS_SYNTAX_ENUM_VARIANT:
    *kind = XS_HIR_MEMBER_VARIANT;
    return true;
  case XS_SYNTAX_DECL_CLASS:
  case XS_SYNTAX_DECL_INTERFACE:
  case XS_SYNTAX_DECL_ENUM:
  case XS_SYNTAX_DECL_DATA:
    *kind = XS_HIR_MEMBER_NESTED_TYPE;
    return true;
  default:
    return false;
  }
}

static const XsSyntaxNode *member_name_node(const XsSyntaxNode *node, XsHirMemberKind kind)
{
  if(kind != XS_HIR_MEMBER_DESTRUCTOR)
    return declaration_name_node(node);
  for(size_t i = node->child_count; i > 0; --i)
  {
    if(node->children[i - 1]->kind == XS_SYNTAX_IDENTIFIER)
      return node->children[i - 1];
  }
  return nullptr;
}

static bool member_can_merge(const XsHirMemberSymbol *previous, XsHirMemberKind kind, const XsSyntaxNode *node)
{
  return previous != nullptr && ((previous->kind == XS_HIR_MEMBER_METHOD && kind == XS_HIR_MEMBER_METHOD) ||
                                 (previous->kind == XS_HIR_MEMBER_CONSTRUCTOR && kind == XS_HIR_MEMBER_CONSTRUCTOR) ||
                                 (previous->kind == XS_HIR_MEMBER_VARIANT && kind == XS_HIR_MEMBER_VARIANT &&
                                  (node->flags & XS_SYNTAX_FLAG_ENUM_VARIANT_OVERLOAD) != 0));
}

static bool collect_member_declaration(const XsSyntaxNode *node, const XsHirSymbol *owner,
                                       XsHirMemberSymbolTable *table, XsDiagnostics *diagnostics)
{
  XsHirMemberKind kind;
  if(!member_kind_for_node(node->kind, &kind))
    return true;
  const XsSyntaxNode *name_node = member_name_node(node, kind);
  if(name_node == nullptr)
    return true;
  char *name = xs_hir_copy_text(name_node->text);
  char *owner_name = xs_hir_copy_cstr(owner->qualified_name);
  if(name == nullptr || owner_name == nullptr)
  {
    free(name);
    free(owner_name);
    table->allocation_failed = true;
    return false;
  }
  const XsHirMemberSymbol *previous = find_member_forward(table, owner->qualified_name, name);
  if(previous != nullptr && !member_can_merge(previous, kind, node))
  {
    report_duplicate_member(diagnostics, name_node, previous);
    free(name);
    free(owner_name);
    return true;
  }
  char *qualified = xs_hir_join_qualified(owner->qualified_name, name);
  if(qualified == nullptr)
  {
    free(name);
    free(owner_name);
    table->allocation_failed = true;
    return false;
  }
  XsHirMemberSymbol symbol = {.kind = kind,
                              .name = name,
                              .owner_qualified_name = owner_name,
                              .qualified_name = qualified,
                              .visibility = member_visibility(node),
                              .span = name_node->span,
                              .syntax = node};
  if(!append_member_symbol(table, symbol))
  {
    free(name);
    free(owner_name);
    free(qualified);
    return false;
  }
  return true;
}

void xs_hir_symbol_table_init(XsHirSymbolTable *table)
{
  *table = (XsHirSymbolTable){0};
}

void xs_hir_symbol_table_free(XsHirSymbolTable *table)
{
  for(size_t i = 0; i < table->count; ++i)
  {
    free(table->symbols[i].name);
    free(table->symbols[i].namespace_name);
    free(table->symbols[i].qualified_name);
  }
  free(table->symbols);
  *table = (XsHirSymbolTable){0};
}

void xs_hir_member_symbol_table_init(XsHirMemberSymbolTable *table)
{
  *table = (XsHirMemberSymbolTable){0};
}

void xs_hir_member_symbol_table_free(XsHirMemberSymbolTable *table)
{
  for(size_t i = 0; i < table->count; ++i)
  {
    free(table->members[i].name);
    free(table->members[i].owner_qualified_name);
    free(table->members[i].qualified_name);
  }
  free(table->members);
  *table = (XsHirMemberSymbolTable){0};
}

const XsHirSymbol *xs_hir_symbol_table_find(const XsHirSymbolTable *table, const char *qualified_name)
{
  if(table == nullptr || qualified_name == nullptr)
    return nullptr;
  for(size_t i = 0; i < table->count; ++i)
  {
    if(strcmp(table->symbols[i].qualified_name, qualified_name) == 0)
      return &table->symbols[i];
  }
  return nullptr;
}

const XsHirMemberSymbol *xs_hir_member_symbol_table_find(const XsHirMemberSymbolTable *table,
                                                         const char *owner_qualified_name, const char *name)
{
  if(table == nullptr || owner_qualified_name == nullptr || name == nullptr)
    return nullptr;
  for(size_t i = table->count; i > 0; --i)
  {
    if(strcmp(table->members[i - 1].owner_qualified_name, owner_qualified_name) == 0 &&
       strcmp(table->members[i - 1].name, name) == 0)
      return &table->members[i - 1];
  }
  return nullptr;
}

bool xs_hir_collect_symbols(const XsSyntaxTree *tree, XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  return xs_hir_collect_symbols_expanded(tree, nullptr, table, diagnostics);
}

bool xs_hir_collect_symbols_expanded(const XsSyntaxTree *tree, const XsMacroDeclarationExpansionSet *macro_declarations,
                                     XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  if(tree == nullptr || tree->root == nullptr || table == nullptr || diagnostics == nullptr)
    return false;
  char *module_name = xs_hir_copy_cstr("");
  char *current_namespace = xs_hir_copy_cstr("");
  if(module_name == nullptr || current_namespace == nullptr)
  {
    free(module_name);
    free(current_namespace);
    table->allocation_failed = true;
    return false;
  }
  XsMacroExpandedDeclarationSet expanded = {0};
  if(!xs_macro_expand_top_level_declarations(tree, macro_declarations, diagnostics, &expanded))
  {
    free(module_name);
    free(current_namespace);
    return false;
  }
  bool public_namespace = tree_has_public_namespace(tree);
  for(size_t i = 0; i < expanded.count; ++i)
  {
    const XsSyntaxNode *child = expanded.items[i].declaration;
    if(child->kind == XS_SYNTAX_DECL_MODULE)
    {
      char *path = xs_hir_path_to_string(xs_hir_first_child_kind(child, XS_SYNTAX_PATH));
      if(path == nullptr)
      {
        table->allocation_failed = true;
        break;
      }
      free(module_name);
      free(current_namespace);
      module_name = path;
      current_namespace = xs_hir_copy_cstr(module_name);
      if(current_namespace == nullptr)
      {
        table->allocation_failed = true;
        break;
      }
      continue;
    }
    if(child->kind == XS_SYNTAX_DECL_NAMESPACE)
    {
      char *path = xs_hir_path_to_string(xs_hir_first_child_kind(child, XS_SYNTAX_PATH));
      if(path == nullptr)
      {
        table->allocation_failed = true;
        break;
      }
      free(current_namespace);
      current_namespace = xs_hir_join_qualified(module_name, path);
      free(path);
      if(current_namespace == nullptr)
      {
        table->allocation_failed = true;
        break;
      }
      continue;
    }
    if(child->kind == XS_SYNTAX_DECL_EXTERN_BLOCK)
    {
      if(!collect_extern_block(child, current_namespace, public_namespace, table, diagnostics))
        break;
      continue;
    }
    if(!collect_declaration(child, current_namespace, public_namespace, table, diagnostics))
      break;
  }
  xs_macro_expanded_declaration_set_free(&expanded);
  free(module_name);
  free(current_namespace);
  if(table->allocation_failed)
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while collecting HIR symbols");
  return !xs_diagnostics_has_error(diagnostics);
}

const char *xs_hir_symbol_kind_name(XsHirSymbolKind kind)
{
  static const char *const names[] = {
      "function", "class", "interface", "enum", "data", "macro", "extern global",
  };
  if((size_t)kind >= sizeof(names) / sizeof(names[0]))
    return "symbol";
  return names[kind];
}

const char *xs_hir_member_kind_name(XsHirMemberKind kind)
{
  static const char *const names[] = {
      "field", "method", "constructor", "destructor", "nested type", "variant",
  };
  if((size_t)kind >= sizeof(names) / sizeof(names[0]))
    return "member";
  return names[kind];
}

bool xs_hir_collect_member_symbols(const XsHirSymbol *owner, const XsMacroDeclarationExpansionSet *macro_declarations,
                                   XsHirMemberSymbolTable *table, XsDiagnostics *diagnostics)
{
  if(owner == nullptr || owner->syntax == nullptr || table == nullptr || diagnostics == nullptr)
    return false;
  if(owner->kind != XS_HIR_SYMBOL_CLASS && owner->kind != XS_HIR_SYMBOL_INTERFACE &&
     owner->kind != XS_HIR_SYMBOL_ENUM && owner->kind != XS_HIR_SYMBOL_DATA)
    return true;
  XsMacroExpandedDeclarationSet expanded = {0};
  if(!xs_macro_expand_child_declarations(owner->syntax, macro_declarations, diagnostics, &expanded))
    return false;
  for(size_t i = 0; i < expanded.count; ++i)
  {
    if(!collect_member_declaration(expanded.items[i].declaration, owner, table, diagnostics))
      break;
  }
  xs_macro_expanded_declaration_set_free(&expanded);
  if(table->allocation_failed)
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while collecting HIR member symbols");
  return !xs_diagnostics_has_error(diagnostics);
}
