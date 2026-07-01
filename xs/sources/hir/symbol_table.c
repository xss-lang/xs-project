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
  switch (syntax) {
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

static bool append_symbol(XsHirSymbolTable *table, XsHirSymbol symbol)
{
  if (table->count == table->capacity) {
    size_t capacity = table->capacity == 0 ? 16 : table->capacity * 2;
    XsHirSymbol *symbols = realloc(table->symbols, capacity * sizeof(*symbols));
    if (symbols == nullptr) {
      table->allocation_failed = true;
      return false;
    }
    table->symbols = symbols;
    table->capacity = capacity;
  }
  table->symbols[table->count++] = symbol;
  return true;
}

static const XsHirSymbol *find_in_namespace(const XsHirSymbolTable *table, const char *namespace_name, const char *name)
{
  for (size_t i = 0; i < table->count; ++i) {
    if (strcmp(table->symbols[i].namespace_name, namespace_name) == 0 && strcmp(table->symbols[i].name, name) == 0)
      return &table->symbols[i];
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

static bool tree_has_public_namespace(const XsSyntaxTree *tree)
{
  for (size_t i = 0; i < tree->root->child_count; ++i) {
    const XsSyntaxNode *child = tree->root->children[i];
    if (child->kind == XS_SYNTAX_DECL_NAMESPACE && child->visibility == XS_SYNTAX_VISIBILITY_PUBLIC)
      return true;
  }
  return false;
}

static XsSyntaxVisibility declaration_visibility(const XsSyntaxNode *node, bool public_namespace)
{
  if (public_namespace && node->visibility == XS_SYNTAX_VISIBILITY_DEFAULT)
    return XS_SYNTAX_VISIBILITY_PUBLIC;
  return node->visibility;
}

static bool collect_declaration(const XsSyntaxNode *node, const char *namespace_name, bool public_namespace,
                                XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  XsHirSymbolKind kind;
  if (!symbol_kind_for_node(node->kind, &kind))
    return true;
  const XsSyntaxNode *name_node = declaration_name_node(node);
  if (name_node == nullptr)
    return true;
  char *name = xs_hir_copy_text(name_node->text);
  char *namespace_copy = xs_hir_copy_cstr(namespace_name);
  if (name == nullptr || namespace_copy == nullptr) {
    free(name);
    free(namespace_copy);
    table->allocation_failed = true;
    return false;
  }
  const XsHirSymbol *previous = find_in_namespace(table, namespace_name, name);
  if (previous != nullptr) {
    report_duplicate(diagnostics, name_node, previous);
    free(name);
    free(namespace_copy);
    return true;
  }
  char *qualified = xs_hir_join_qualified(namespace_name, name);
  if (qualified == nullptr) {
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
      .visibility = declaration_visibility(node, public_namespace),
      .span = name_node->span,
      .syntax = node,
  };
  if (!append_symbol(table, symbol)) {
    free(name);
    free(namespace_copy);
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
  for (size_t i = 0; i < table->count; ++i) {
    free(table->symbols[i].name);
    free(table->symbols[i].namespace_name);
    free(table->symbols[i].qualified_name);
  }
  free(table->symbols);
  *table = (XsHirSymbolTable){0};
}

const XsHirSymbol *xs_hir_symbol_table_find(const XsHirSymbolTable *table, const char *qualified_name)
{
  if (table == nullptr || qualified_name == nullptr)
    return nullptr;
  for (size_t i = 0; i < table->count; ++i) {
    if (strcmp(table->symbols[i].qualified_name, qualified_name) == 0)
      return &table->symbols[i];
  }
  return nullptr;
}

bool xs_hir_collect_symbols(const XsSyntaxTree *tree, XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  return xs_hir_collect_symbols_expanded(tree, nullptr, table, diagnostics);
}

bool xs_hir_collect_symbols_expanded(const XsSyntaxTree *tree,
                                     const XsMacroDeclarationExpansionSet *macro_declarations,
                                     XsHirSymbolTable *table, XsDiagnostics *diagnostics)
{
  if (tree == nullptr || tree->root == nullptr || table == nullptr || diagnostics == nullptr)
    return false;
  char *module_name = xs_hir_copy_cstr("");
  char *current_namespace = xs_hir_copy_cstr("");
  if (module_name == nullptr || current_namespace == nullptr) {
    free(module_name);
    free(current_namespace);
    table->allocation_failed = true;
    return false;
  }
  XsMacroExpandedDeclarationSet expanded = {0};
  if (!xs_macro_expand_top_level_declarations(tree, macro_declarations, diagnostics, &expanded)) {
    free(module_name);
    free(current_namespace);
    return false;
  }
  bool public_namespace = tree_has_public_namespace(tree);
  for (size_t i = 0; i < expanded.count; ++i) {
    const XsSyntaxNode *child = expanded.items[i].declaration;
    if (child->kind == XS_SYNTAX_DECL_MODULE) {
      char *path = xs_hir_path_to_string(xs_hir_first_child_kind(child, XS_SYNTAX_PATH));
      if (path == nullptr) {
        table->allocation_failed = true;
        break;
      }
      free(module_name);
      free(current_namespace);
      module_name = path;
      current_namespace = xs_hir_copy_cstr(module_name);
      if (current_namespace == nullptr) {
        table->allocation_failed = true;
        break;
      }
      continue;
    }
    if (child->kind == XS_SYNTAX_DECL_NAMESPACE) {
      char *path = xs_hir_path_to_string(xs_hir_first_child_kind(child, XS_SYNTAX_PATH));
      if (path == nullptr) {
        table->allocation_failed = true;
        break;
      }
      free(current_namespace);
      current_namespace = xs_hir_join_qualified(module_name, path);
      free(path);
      if (current_namespace == nullptr) {
        table->allocation_failed = true;
        break;
      }
      continue;
    }
    if (!collect_declaration(child, current_namespace, public_namespace, table, diagnostics))
      break;
  }
  xs_macro_expanded_declaration_set_free(&expanded);
  free(module_name);
  free(current_namespace);
  if (table->allocation_failed)
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while collecting HIR symbols");
  return !xs_diagnostics_has_error(diagnostics);
}

const char *xs_hir_symbol_kind_name(XsHirSymbolKind kind)
{
  static const char *const names[] = {
      "function", "class", "interface", "enum", "data", "macro",
  };
  if ((size_t)kind >= sizeof(names) / sizeof(names[0]))
    return "symbol";
  return names[kind];
}
