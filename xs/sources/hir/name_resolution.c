/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

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
  if(node == nullptr)
    return false;
  if((node->kind == XS_SYNTAX_PARAMETER || node->kind == XS_SYNTAX_DECL_VARIABLE) && node->child_count != 0 &&
     node->children[0]->kind == XS_SYNTAX_IDENTIFIER && text_matches_cstr(node->children[0]->text, name))
    return true;
  for(size_t i = 0; i < node->child_count; ++i)
  {
    if(local_name_declared(node->children[i], name))
      return true;
  }
  return false;
}

static const XsSyntaxNode *local_declaration_type(const XsSyntaxNode *node, const char *name)
{
  if(node == nullptr)
    return nullptr;
  if((node->kind == XS_SYNTAX_PARAMETER || node->kind == XS_SYNTAX_DECL_VARIABLE) && node->child_count >= 2 &&
     node->children[0]->kind == XS_SYNTAX_IDENTIFIER && text_matches_cstr(node->children[0]->text, name))
    return node->children[1];
  for(size_t i = 0; i < node->child_count; ++i)
  {
    const XsSyntaxNode *type = local_declaration_type(node->children[i], name);
    if(type != nullptr)
      return type;
  }
  return nullptr;
}

static char *join_member_path(char *base, XsText member)
{
  char *name = xs_hir_copy_text(member);
  if(name == nullptr)
  {
    free(base);
    return nullptr;
  }
  char *result = xs_hir_join_qualified(base, name);
  free(base);
  free(name);
  return result;
}

static char *expression_path(const XsSyntaxNode *node)
{
  if(node == nullptr)
    return nullptr;
  if(node->kind == XS_SYNTAX_EXPR_IDENTIFIER)
  {
    const XsSyntaxNode *path = xs_hir_first_child_kind(node, XS_SYNTAX_PATH);
    if(path != nullptr)
      return xs_hir_path_to_string(path);
    const XsSyntaxNode *identifier = xs_hir_first_child_kind(node, XS_SYNTAX_IDENTIFIER);
    return identifier == nullptr ? nullptr : xs_hir_copy_text(identifier->text);
  }
  if(node->kind == XS_SYNTAX_EXPR_MEMBER_ACCESS || node->kind == XS_SYNTAX_EXPR_METHOD_CALL)
  {
    if(node->child_count < 2 || node->children[1]->kind != XS_SYNTAX_IDENTIFIER)
      return nullptr;
    char *base = expression_path(node->children[0]);
    if(base == nullptr)
      return nullptr;
    return join_member_path(base, node->children[1]->text);
  }
  return nullptr;
}

static bool first_segment_is_local(const XsSyntaxNode *function, const char *path)
{
  const char *dot = strchr(path, '.');
  size_t length = dot == nullptr ? strlen(path) : (size_t)(dot - path);
  char *first = malloc(length + 1);
  if(first == nullptr)
    return false;
  memcpy(first, path, length);
  first[length] = '\0';
  bool result = local_name_declared(function, first);
  free(first);
  return result;
}

static bool symbol_visible_from(const XsHirSymbol *symbol, const char *current_namespace, uint64_t current_file_id)
{
  if(symbol == nullptr)
    return false;
  if(symbol->visibility == XS_SYNTAX_VISIBILITY_PUBLIC)
    return true;
  return strcmp(symbol->namespace_name, current_namespace) == 0 && symbol->span.file_id == current_file_id;
}

static bool same_root_module(const char *left, const char *right)
{
  size_t left_length = strcspn(left, ".");
  size_t right_length = strcspn(right, ".");
  return left_length == right_length && strncmp(left, right, left_length) == 0;
}

static bool symbol_module_available(const XsHirSymbol *symbol, const char *current_namespace,
                                    const XsHirImportScope *imports)
{
  return symbol == nullptr || same_root_module(symbol->namespace_name, current_namespace) ||
         xs_hir_import_scope_has_module(imports, symbol->namespace_name);
}

static const XsHirSymbol *resolve_name_use(const char *path, const char *current_namespace,
                                           const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports)
{
  const XsHirImportBinding *binding = xs_hir_import_scope_find(imports, path);
  if(binding != nullptr)
    return binding->symbol;
  if(strchr(path, '.') != nullptr)
    return xs_hir_symbol_table_find(project_symbols, path);
  char *qualified = xs_hir_join_qualified(current_namespace, path);
  if(qualified == nullptr)
    return nullptr;
  const XsHirSymbol *symbol = xs_hir_symbol_table_find(project_symbols, qualified);
  free(qualified);
  return symbol;
}

static const XsHirSymbol *resolve_type_symbol(const XsSyntaxNode *type, const char *current_namespace,
                                              const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports)
{
  if(type == nullptr || type->kind != XS_SYNTAX_TYPE_NAMED)
    return nullptr;
  char *path = xs_hir_path_to_string(xs_hir_first_child_kind(type, XS_SYNTAX_PATH));
  if(path == nullptr)
    return nullptr;
  const XsHirSymbol *symbol = resolve_name_use(path, current_namespace, project_symbols, imports);
  free(path);
  if(symbol == nullptr || (symbol->kind != XS_HIR_SYMBOL_CLASS && symbol->kind != XS_HIR_SYMBOL_INTERFACE &&
                           symbol->kind != XS_HIR_SYMBOL_DATA))
    return nullptr;
  return symbol;
}

static char *identifier_expression_name(const XsSyntaxNode *node)
{
  if(node == nullptr || node->kind != XS_SYNTAX_EXPR_IDENTIFIER)
    return nullptr;
  const XsSyntaxNode *identifier = xs_hir_first_child_kind(node, XS_SYNTAX_IDENTIFIER);
  return identifier == nullptr ? nullptr : xs_hir_copy_text(identifier->text);
}

static bool report_missing_method(XsDiagnostics *diagnostics, const XsSyntaxNode *method_name,
                                  const XsHirSymbol *receiver_type)
{
  char *name = xs_hir_copy_text(method_name->text);
  if(name == nullptr)
    return false;
  char message[512];
  snprintf(message, sizeof(message), "method '%s' does not exist on type '%s'", name, receiver_type->qualified_name);
  free(name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(method_name), message);
}

static bool validate_method_call_target(const XsSyntaxNode *call, const XsSyntaxNode *function,
                                        const char *namespace_name, const XsHirSymbolTable *project_symbols,
                                        const XsHirImportScope *imports, const XsHirMemberSymbolTable *members,
                                        XsDiagnostics *diagnostics)
{
  if(call == nullptr || call->kind != XS_SYNTAX_EXPR_METHOD_CALL || call->child_count < 2 ||
     call->children[1]->kind != XS_SYNTAX_IDENTIFIER)
    return true;
  char *receiver = identifier_expression_name(call->children[0]);
  if(receiver == nullptr)
    return true;
  const XsSyntaxNode *receiver_type_node = local_declaration_type(function, receiver);
  free(receiver);
  const XsHirSymbol *receiver_type = resolve_type_symbol(receiver_type_node, namespace_name, project_symbols, imports);
  if(receiver_type == nullptr)
    return true;
  const XsSyntaxNode *method_name = call->children[1];
  char *method = xs_hir_copy_text(method_name->text);
  if(method == nullptr)
    return false;
  const XsHirMemberSymbol *member = xs_hir_member_symbol_table_find(members, receiver_type->qualified_name, method);
  free(method);
  if(member != nullptr && member->kind == XS_HIR_MEMBER_METHOD)
    return true;
  return report_missing_method(diagnostics, method_name, receiver_type);
}

static bool validate_call_target(const XsSyntaxNode *target, const XsSyntaxNode *function, const char *namespace_name,
                                 uint64_t current_file_id, const XsHirSymbolTable *project_symbols,
                                 const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  char *path = expression_path(target);
  if(path == nullptr)
    return true;
  if(first_segment_is_local(function, path))
  {
    free(path);
    return true;
  }
  const XsHirSymbol *symbol = resolve_name_use(path, namespace_name, project_symbols, imports);
  if(symbol == nullptr)
  {
    char message[512];
    snprintf(message, sizeof(message), "name '%s' does not resolve in HIR symbol or import scope", path);
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(target), message);
    free(path);
    return false;
  }
  if(!symbol_visible_from(symbol, namespace_name, current_file_id))
  {
    char message[512];
    snprintf(message, sizeof(message), "name '%s' is not visible from namespace '%s'", path, namespace_name);
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(target), message);
    free(path);
    return false;
  }
  if(strchr(path, '.') != nullptr && !symbol_module_available(symbol, namespace_name, imports))
  {
    char message[512];
    snprintf(message, sizeof(message), "module for name '%s' is not imported", path);
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_hir_node_span(target), message);
    free(path);
    return false;
  }
  free(path);
  return true;
}

static bool has_statement_macro_child(const XsSyntaxNode *node)
{
  if(node == nullptr)
    return false;
  for(size_t i = 0; i < node->child_count; ++i)
  {
    if(node->children[i]->kind == XS_SYNTAX_STMT_MACRO_CALL)
      return true;
  }
  return false;
}

static bool validate_name_uses_in_node(const XsSyntaxNode *node, const XsSyntaxNode *function,
                                       const char *namespace_name, uint64_t current_file_id,
                                       const XsMacroStatementExpansionSet *macro_statements,
                                       const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports,
                                       const XsHirMemberSymbolTable *members, XsDiagnostics *diagnostics)
{
  if(node == nullptr)
    return true;
  bool matched_macro = false;
  bool macro_success = true;
  for(size_t i = 0; macro_statements != nullptr && i < macro_statements->count; ++i)
  {
    if(!xs_macro_statement_expansion_matches(&macro_statements->items[i], node))
      continue;
    matched_macro = true;
    macro_success =
        validate_name_uses_in_node(macro_statements->items[i].statement, function, namespace_name, current_file_id,
                                   macro_statements, project_symbols, imports, members, diagnostics) &&
        macro_success;
  }
  if(matched_macro)
    return macro_success;
  bool success = true;
  if(node->kind == XS_SYNTAX_EXPR_CALL && node->child_count != 0)
    success = validate_call_target(node->children[0], function, namespace_name, current_file_id, project_symbols,
                                   imports, diagnostics);
  else if(node->kind == XS_SYNTAX_EXPR_METHOD_CALL)
  {
    success =
        validate_method_call_target(node, function, namespace_name, project_symbols, imports, members, diagnostics) &&
        success;
    success =
        validate_call_target(node, function, namespace_name, current_file_id, project_symbols, imports, diagnostics) &&
        success;
  }
  if(macro_statements != nullptr && has_statement_macro_child(node))
  {
    XsMacroExpandedStatementSet expanded = {0};
    if(!xs_macro_expand_child_statements(node, macro_statements, diagnostics, &expanded))
      return false;
    for(size_t i = 0; i < expanded.count; ++i)
      success = validate_name_uses_in_node(expanded.items[i].statement, function, namespace_name, current_file_id,
                                           macro_statements, project_symbols, imports, members, diagnostics) &&
                success;
    xs_macro_expanded_statement_set_free(&expanded);
    return success;
  }
  for(size_t i = 0; i < node->child_count; ++i)
    success = validate_name_uses_in_node(node->children[i], function, namespace_name, current_file_id, macro_statements,
                                         project_symbols, imports, members, diagnostics) &&
              success;
  return success;
}

bool xs_hir_validate_name_uses_with_macros(const XsSyntaxTree *tree,
                                           const XsMacroDeclarationExpansionSet *macro_declarations,
                                           const XsMacroStatementExpansionSet *macro_statements,
                                           const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports,
                                           XsDiagnostics *diagnostics)
{
  if(tree == nullptr || tree->root == nullptr || project_symbols == nullptr || imports == nullptr ||
     diagnostics == nullptr)
    return false;
  XsHirMemberSymbolTable members;
  xs_hir_member_symbol_table_init(&members);
  bool success = true;
  for(size_t i = 0; i < project_symbols->count; ++i)
    success = xs_hir_collect_member_symbols(&project_symbols->symbols[i], macro_declarations, &members, diagnostics) &&
              success;
  for(size_t i = 0; i < project_symbols->count; ++i)
  {
    const XsHirSymbol *symbol = &project_symbols->symbols[i];
    if(symbol->kind != XS_HIR_SYMBOL_FUNCTION || symbol->span.file_id != tree->file_id)
      continue;
    success = validate_name_uses_in_node(symbol->syntax, symbol->syntax, symbol->namespace_name, tree->file_id,
                                         macro_statements, project_symbols, imports, &members, diagnostics) &&
              success;
  }
  xs_hir_member_symbol_table_free(&members);
  return success && !xs_diagnostics_has_error(diagnostics);
}

bool xs_hir_validate_name_uses_expanded(const XsSyntaxTree *tree, const XsMacroStatementExpansionSet *macro_statements,
                                        const XsHirSymbolTable *project_symbols, const XsHirImportScope *imports,
                                        XsDiagnostics *diagnostics)
{
  return xs_hir_validate_name_uses_with_macros(tree, nullptr, macro_statements, project_symbols, imports, diagnostics);
}

bool xs_hir_validate_name_uses(const XsSyntaxTree *tree, const XsHirSymbolTable *project_symbols,
                               const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  return xs_hir_validate_name_uses_expanded(tree, nullptr, project_symbols, imports, diagnostics);
}
