/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/hir/symbol_table.h"

#include "syntax_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool append_import_binding(XsHirImportScope *scope, XsHirImportBinding binding)
{
  if (scope->count == scope->capacity)
  {
    size_t capacity = scope->capacity == 0 ? 16 : scope->capacity * 2;
    XsHirImportBinding *bindings = realloc(scope->bindings, capacity * sizeof(*bindings));
    if (bindings == NULL)
    {
      scope->allocation_failed = true;
      return false;
    }
    scope->bindings = bindings;
    scope->capacity = capacity;
  }
  scope->bindings[scope->count++] = binding;
  return true;
}

static bool report_import_conflict(XsDiagnostics *diagnostics, XsSpan span, const char *local_name)
{
  char message[512];
  snprintf(message, sizeof(message), "imported name '%s' conflicts with a previous import", local_name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span, message);
}

static bool import_symbol_as(XsHirImportScope *scope, const XsHirSymbol *symbol, const char *local_name,
                             XsSourceSpan source_span, XsDiagnostics *diagnostics)
{
  char *copy = xs_hir_copy_cstr(local_name);
  if (copy == NULL)
  {
    scope->allocation_failed = true;
    return false;
  }
  if (xs_hir_import_scope_find(scope, local_name) != NULL)
  {
    XsSpan span = {.start = source_span.start_offset, .end = source_span.end_offset};
    report_import_conflict(diagnostics, span, local_name);
    free(copy);
    return true;
  }
  XsHirImportBinding binding = {
      .local_name = copy,
      .symbol = symbol,
      .span = source_span,
  };
  if (!append_import_binding(scope, binding))
  {
    free(copy);
    return false;
  }
  return true;
}

static bool import_module_namespace(const XsSyntaxNode *import_node, const XsSyntaxNode *path,
                                    const XsHirSymbolTable *project_symbols, XsHirImportScope *scope,
                                    XsDiagnostics *diagnostics)
{
  char *module_name = xs_hir_path_to_string(path);
  if (module_name == NULL)
  {
    scope->allocation_failed = true;
    return false;
  }
  for (size_t i = 0; i < project_symbols->count; ++i)
  {
    if (!xs_hir_symbol_is_public_child_of_namespace(&project_symbols->symbols[i], module_name))
      continue;
    char *local_name = xs_hir_join_qualified(module_name, project_symbols->symbols[i].name);
    if (local_name == NULL)
    {
      free(module_name);
      scope->allocation_failed = true;
      return false;
    }
    bool ok = import_symbol_as(scope, &project_symbols->symbols[i], local_name, import_node->span, diagnostics);
    free(local_name);
    if (!ok)
    {
      free(module_name);
      return false;
    }
  }
  free(module_name);
  return true;
}

static const XsHirSymbol *find_symbol_in_namespace(const XsHirSymbolTable *table, const char *namespace_name,
                                                   const char *name)
{
  for (size_t i = 0; i < table->count; ++i)
  {
    if (xs_hir_symbol_is_public_child_of_namespace(&table->symbols[i], namespace_name) &&
        strcmp(table->symbols[i].name, name) == 0)
      return &table->symbols[i];
  }
  return NULL;
}

static bool import_selected_name(const XsSyntaxNode *name_node, const char *module_name,
                                 const XsHirSymbolTable *project_symbols, XsHirImportScope *scope,
                                 XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *imported = xs_hir_first_child_kind(name_node, XS_SYNTAX_IDENTIFIER);
  if (imported == NULL)
    return true;
  char *imported_name = xs_hir_copy_text(imported->text);
  const XsSyntaxNode *alias = xs_hir_second_child_kind(name_node, XS_SYNTAX_IDENTIFIER);
  char *local_name = alias == NULL ? xs_hir_copy_text(imported->text) : xs_hir_copy_text(alias->text);
  if (imported_name == NULL || local_name == NULL)
  {
    free(imported_name);
    free(local_name);
    scope->allocation_failed = true;
    return false;
  }
  const XsHirSymbol *symbol = find_symbol_in_namespace(project_symbols, module_name, imported_name);
  if (symbol == NULL)
  {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(imported),
                       "selected import does not name a public symbol in the imported module");
    free(imported_name);
    free(local_name);
    return true;
  }
  bool ok = import_symbol_as(scope, symbol, local_name, name_node->span, diagnostics);
  free(imported_name);
  free(local_name);
  return ok;
}

static bool import_selected(const XsSyntaxNode *import_node, const XsHirSymbolTable *project_symbols,
                            XsHirImportScope *scope, XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *path = xs_hir_first_child_kind(import_node, XS_SYNTAX_PATH);
  char *module_name = xs_hir_path_to_string(path);
  if (module_name == NULL)
  {
    scope->allocation_failed = true;
    return false;
  }
  bool ok = true;
  if ((import_node->flags & XS_SYNTAX_FLAG_WILDCARD) != 0)
  {
    for (size_t i = 0; i < project_symbols->count; ++i)
    {
      if (!xs_hir_symbol_is_public_child_of_namespace(&project_symbols->symbols[i], module_name))
        continue;
      ok = import_symbol_as(scope, &project_symbols->symbols[i], project_symbols->symbols[i].name, import_node->span,
                            diagnostics) &&
           ok;
    }
  }
  else
  {
    for (size_t i = 0; i < import_node->child_count; ++i)
    {
      if (import_node->children[i]->kind == XS_SYNTAX_IMPORT_NAME)
        ok = import_selected_name(import_node->children[i], module_name, project_symbols, scope, diagnostics) && ok;
    }
  }
  free(module_name);
  return ok;
}

void xs_hir_import_scope_init(XsHirImportScope *scope)
{
  *scope = (XsHirImportScope){0};
}

void xs_hir_import_scope_free(XsHirImportScope *scope)
{
  for (size_t i = 0; i < scope->count; ++i)
    free(scope->bindings[i].local_name);
  free(scope->bindings);
  *scope = (XsHirImportScope){0};
}

const XsHirImportBinding *xs_hir_import_scope_find(const XsHirImportScope *scope, const char *local_name)
{
  if (scope == NULL || local_name == NULL)
    return NULL;
  for (size_t i = 0; i < scope->count; ++i)
  {
    if (strcmp(scope->bindings[i].local_name, local_name) == 0)
      return &scope->bindings[i];
  }
  return NULL;
}

bool xs_hir_resolve_imports(const XsSyntaxTree *tree, const XsHirSymbolTable *project_symbols, XsHirImportScope *scope,
                            XsDiagnostics *diagnostics)
{
  if (tree == NULL || tree->root == NULL || project_symbols == NULL || scope == NULL || diagnostics == NULL)
    return false;
  bool success = true;
  for (size_t i = 0; i < tree->root->child_count; ++i)
  {
    const XsSyntaxNode *child = tree->root->children[i];
    if (child->kind != XS_SYNTAX_DECL_IMPORT)
      continue;
    if (xs_hir_first_child_kind(child, XS_SYNTAX_IMPORT_NAME) != NULL || (child->flags & XS_SYNTAX_FLAG_WILDCARD) != 0)
      success = import_selected(child, project_symbols, scope, diagnostics) && success;
    else
    {
      for (size_t j = 0; j < child->child_count; ++j)
      {
        if (child->children[j]->kind == XS_SYNTAX_PATH)
          success = import_module_namespace(child, child->children[j], project_symbols, scope, diagnostics) && success;
      }
    }
  }
  if (scope->allocation_failed)
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while resolving HIR imports");
  return success && !xs_diagnostics_has_error(diagnostics);
}
