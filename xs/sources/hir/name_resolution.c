#include "xs/hir/symbol_table.h"

#include "syntax_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool text_matches_cstr(XsText text, const char *name)
{
  size_t length = strlen(name);
  return text.length == length && memcmp(text.data, name, length) == 0;
}

static bool local_name_declared(const XsSyntaxNode *node, const char *name)
{
  if (node == NULL)
    return false;
  if ((node->kind == XS_SYNTAX_PARAMETER || node->kind == XS_SYNTAX_DECL_VARIABLE) && node->child_count != 0 &&
      node->children[0]->kind == XS_SYNTAX_IDENTIFIER && text_matches_cstr(node->children[0]->text, name))
    return true;
  for (size_t i = 0; i < node->child_count; ++i) {
    if (local_name_declared(node->children[i], name))
      return true;
  }
  return false;
}

static char *join_member_path(char *base, XsText member)
{
  char *name = xs_hir_copy_text(member);
  if (name == NULL) {
    free(base);
    return NULL;
  }
  char *result = xs_hir_join_qualified(base, name);
  free(base);
  free(name);
  return result;
}

static char *expression_path(const XsSyntaxNode *node)
{
  if (node == NULL)
    return NULL;
  if (node->kind == XS_SYNTAX_EXPR_IDENTIFIER) {
    const XsSyntaxNode *identifier = xs_hir_first_child_kind(node, XS_SYNTAX_IDENTIFIER);
    return identifier == NULL ? NULL : xs_hir_copy_text(identifier->text);
  }
  if (node->kind == XS_SYNTAX_EXPR_MEMBER_ACCESS || node->kind == XS_SYNTAX_EXPR_METHOD_CALL) {
    if (node->child_count < 2 || node->children[1]->kind != XS_SYNTAX_IDENTIFIER)
      return NULL;
    char *base = expression_path(node->children[0]);
    if (base == NULL)
      return NULL;
    return join_member_path(base, node->children[1]->text);
  }
  return NULL;
}

static bool first_segment_is_local(const XsSyntaxNode *function, const char *path)
{
  const char *dot = strchr(path, '.');
  size_t length = dot == NULL ? strlen(path) : (size_t)(dot - path);
  char *first = malloc(length + 1);
  if (first == NULL)
    return false;
  memcpy(first, path, length);
  first[length] = '\0';
  bool result = local_name_declared(function, first);
  free(first);
  return result;
}

static bool symbol_visible_from(const XsHirSymbol *symbol, const char *current_namespace, uint64_t current_file_id)
{
  if (symbol == NULL)
    return false;
  if (symbol->visibility == XS_SYNTAX_VISIBILITY_PUBLIC)
    return true;
  return strcmp(symbol->namespace_name, current_namespace) == 0 && symbol->span.file_id == current_file_id;
}

static const XsHirSymbol *resolve_name_use(const char *path, const char *current_namespace,
                                           const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports)
{
  const XsHirImportBinding *binding = xs_hir_import_scope_find(imports, path);
  if (binding != NULL)
    return binding->symbol;
  if (strchr(path, '.') != NULL)
    return xs_hir_symbol_table_find(project_symbols, path);
  char *qualified = xs_hir_join_qualified(current_namespace, path);
  if (qualified == NULL)
    return NULL;
  const XsHirSymbol *symbol = xs_hir_symbol_table_find(project_symbols, qualified);
  free(qualified);
  return symbol;
}

static bool validate_call_target(const XsSyntaxNode *target, const XsSyntaxNode *function, const char *namespace_name,
                                 uint64_t current_file_id, const XsHirSymbolTable *project_symbols,
                                 const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  char *path = expression_path(target);
  if (path == NULL)
    return true;
  if (first_segment_is_local(function, path)) {
    free(path);
    return true;
  }
  const XsHirSymbol *symbol = resolve_name_use(path, namespace_name, project_symbols, imports);
  if (symbol == NULL) {
    char message[512];
    snprintf(message, sizeof(message), "name '%s' does not resolve in HIR symbol or import scope", path);
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(target), message);
    free(path);
    return false;
  }
  if (!symbol_visible_from(symbol, namespace_name, current_file_id)) {
    char message[512];
    snprintf(message, sizeof(message), "name '%s' is not visible from namespace '%s'", path, namespace_name);
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(target), message);
    free(path);
    return false;
  }
  free(path);
  return true;
}

static bool validate_name_uses_in_node(const XsSyntaxNode *node, const XsSyntaxNode *function,
                                       const char *namespace_name, uint64_t current_file_id,
                                       const XsMacroStatementExpansionSet *macro_statements,
                                       const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports,
                                       XsDiagnostics *diagnostics)
{
  if (node == NULL)
    return true;
  const XsSyntaxNode *replacement = xs_macro_statement_expansion_find(macro_statements, node);
  if (replacement != NULL)
    return validate_name_uses_in_node(replacement, function, namespace_name, current_file_id, macro_statements,
                                      project_symbols, imports, diagnostics);
  bool success = true;
  if (node->kind == XS_SYNTAX_EXPR_CALL && node->child_count != 0)
    success = validate_call_target(node->children[0], function, namespace_name, current_file_id, project_symbols,
                                   imports, diagnostics);
  else if (node->kind == XS_SYNTAX_EXPR_METHOD_CALL)
    success =
        validate_call_target(node, function, namespace_name, current_file_id, project_symbols, imports, diagnostics);
  for (size_t i = 0; i < node->child_count; ++i)
    success = validate_name_uses_in_node(node->children[i], function, namespace_name, current_file_id, macro_statements,
                                         project_symbols, imports, diagnostics) &&
              success;
  return success;
}

bool xs_hir_validate_name_uses_expanded(const XsSyntaxTree *tree, const XsMacroStatementExpansionSet *macro_statements,
                                        const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports,
                                        XsDiagnostics *diagnostics)
{
  if (tree == NULL || tree->root == NULL || project_symbols == NULL || imports == NULL || diagnostics == NULL)
    return false;
  bool success = true;
  for (size_t i = 0; i < project_symbols->count; ++i) {
    const XsHirSymbol *symbol = &project_symbols->symbols[i];
    if (symbol->kind != XS_HIR_SYMBOL_FUNCTION || symbol->span.file_id != tree->file_id)
      continue;
    success = validate_name_uses_in_node(symbol->syntax, symbol->syntax, symbol->namespace_name, tree->file_id,
                                         macro_statements, project_symbols, imports, diagnostics) &&
              success;
  }
  return success && !xs_diagnostics_has_error(diagnostics);
}

bool xs_hir_validate_name_uses(const XsSyntaxTree *tree, const XsHirSymbolTable *project_symbols,
                               const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  return xs_hir_validate_name_uses_expanded(tree, NULL, project_symbols, imports, diagnostics);
}
