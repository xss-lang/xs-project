#include "xs/hir/type_resolution.h"

#include "xs/hir/type_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct GenericScope GenericScope;

struct GenericScope
{
  const GenericScope *parent;
  const XsSyntaxNode *declaration;
};

static const XsSyntaxNode *first_child_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if (node == NULL)
    return NULL;
  for (size_t i = 0; i < node->child_count; ++i) {
    if (node->children[i]->kind == kind)
      return node->children[i];
  }
  return NULL;
}

static char *copy_cstr(const char *text)
{
  size_t length = strlen(text);
  char *copy = malloc(length + 1);
  if (copy != NULL)
    memcpy(copy, text, length + 1);
  return copy;
}

static char *join_qualified(const char *namespace_name, const char *name)
{
  size_t namespace_length = strlen(namespace_name);
  size_t name_length = strlen(name);
  bool has_namespace = namespace_length != 0;
  char *result = malloc(namespace_length + (has_namespace ? 1 : 0) + name_length + 1);
  if (result == NULL)
    return NULL;
  size_t offset = 0;
  if (has_namespace) {
    memcpy(result, namespace_name, namespace_length + 1);
    offset = namespace_length;
    result[offset++] = '.';
  }
  memcpy(result + offset, name, name_length + 1);
  return result;
}

static XsSpan node_span(const XsSyntaxNode *node)
{
  return (XsSpan){.start = node->span.start_offset, .end = node->span.end_offset};
}

static bool text_equal(XsText left, XsText right)
{
  return left.length == right.length && memcmp(left.data, right.data, left.length) == 0;
}

static bool primitive_type_name(XsText name)
{
  return xs_hir_primitive_find(name.data, name.length) != NULL;
}

static const XsSyntaxNode *path_identifier_at(const XsSyntaxNode *path, size_t target)
{
  if (path == NULL || path->kind != XS_SYNTAX_PATH)
    return NULL;
  size_t index = 0;
  for (size_t i = 0; i < path->child_count; ++i) {
    if (path->children[i]->kind != XS_SYNTAX_IDENTIFIER)
      continue;
    if (index == target)
      return path->children[i];
    ++index;
  }
  return NULL;
}

static size_t path_segment_count(const XsSyntaxNode *path)
{
  size_t count = 0;
  if (path == NULL)
    return 0;
  for (size_t i = 0; i < path->child_count; ++i) {
    if (path->children[i]->kind == XS_SYNTAX_IDENTIFIER)
      ++count;
  }
  return count;
}

static char *path_to_string(const XsSyntaxNode *path)
{
  size_t count = path_segment_count(path);
  if (count == 0)
    return copy_cstr("");
  size_t length = count - 1;
  for (size_t i = 0; i < count; ++i)
    length += path_identifier_at(path, i)->text.length;
  char *result = malloc(length + 1);
  if (result == NULL)
    return NULL;
  size_t offset = 0;
  for (size_t i = 0; i < count; ++i) {
    const XsSyntaxNode *identifier = path_identifier_at(path, i);
    if (offset != 0)
      result[offset++] = '.';
    memcpy(result + offset, identifier->text.data, identifier->text.length);
    offset += identifier->text.length;
  }
  result[offset] = '\0';
  return result;
}

static bool generic_scope_contains(const GenericScope *scope, XsText name)
{
  for (const GenericScope *current = scope; current != NULL; current = current->parent) {
    for (size_t i = 0; i < current->declaration->child_count; ++i) {
      const XsSyntaxNode *parameter = current->declaration->children[i];
      if (parameter->kind != XS_SYNTAX_GENERIC_PARAMETER)
        continue;
      const XsSyntaxNode *identifier = first_child_kind(parameter, XS_SYNTAX_IDENTIFIER);
      if (identifier != NULL && text_equal(identifier->text, name))
        return true;
    }
  }
  return false;
}

static bool symbol_is_type(const XsHirSymbol *symbol)
{
  return symbol != NULL && (symbol->kind == XS_HIR_SYMBOL_CLASS || symbol->kind == XS_HIR_SYMBOL_INTERFACE ||
                            symbol->kind == XS_HIR_SYMBOL_ENUM || symbol->kind == XS_HIR_SYMBOL_DATA);
}

static bool symbol_visible_from(const XsHirSymbol *symbol, const char *namespace_name)
{
  return symbol != NULL &&
         (strcmp(symbol->namespace_name, namespace_name) == 0 || symbol->visibility == XS_SYNTAX_VISIBILITY_PUBLIC);
}

static const XsHirSymbol *resolve_user_type(const char *path, const char *namespace_name,
                                            const XsHirSymbolTable *symbols, const XsHirImportScope *imports)
{
  const XsHirImportBinding *binding = xs_hir_import_scope_find(imports, path);
  if (binding != NULL)
    return binding->symbol;
  if (strchr(path, '.') != NULL)
    return xs_hir_symbol_table_find(symbols, path);
  char *qualified = join_qualified(namespace_name, path);
  if (qualified == NULL)
    return NULL;
  const XsHirSymbol *symbol = xs_hir_symbol_table_find(symbols, qualified);
  free(qualified);
  return symbol;
}

static bool report_unresolved_type(XsDiagnostics *diagnostics, const XsSyntaxNode *type, const char *path)
{
  char message[512];
  snprintf(message, sizeof(message), "type '%s' does not resolve to a primitive, generic parameter, or visible type",
           path);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(type), message);
}

static bool report_invisible_type(XsDiagnostics *diagnostics, const XsSyntaxNode *type, const char *path,
                                  const char *namespace_name)
{
  char message[512];
  snprintf(message, sizeof(message), "type '%s' is not visible from namespace '%s'", path, namespace_name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(type), message);
}

static bool resolve_type_node(const XsSyntaxNode *type, const char *namespace_name, const GenericScope *generics,
                              const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                              XsDiagnostics *diagnostics);

static bool report_generic_arity(XsDiagnostics *diagnostics, const XsSyntaxNode *type, const char *path,
                                 size_t expected, size_t actual)
{
  char message[512];
  snprintf(message, sizeof(message), "generic type '%s' expects %zu type argument(s), got %zu", path, expected, actual);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(type), message);
}

static bool report_constraint_kind(XsDiagnostics *diagnostics, const XsSyntaxNode *type, const char *path)
{
  char message[512];
  snprintf(message, sizeof(message), "generic constraint '%s' must resolve to an interface", path);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(type), message);
}

static size_t generic_parameter_count(const XsSyntaxNode *declaration)
{
  size_t count = 0;
  if (declaration == NULL)
    return 0;
  for (size_t i = 0; i < declaration->child_count; ++i) {
    if (declaration->children[i]->kind == XS_SYNTAX_GENERIC_PARAMETER)
      ++count;
  }
  return count;
}

static bool resolve_named_type(const XsSyntaxNode *type, const char *namespace_name, const GenericScope *generics,
                               const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                               XsDiagnostics *diagnostics, bool check_arity, const XsHirSymbol **resolved)
{
  if (resolved != NULL)
    *resolved = NULL;
  const XsSyntaxNode *path = first_child_kind(type, XS_SYNTAX_PATH);
  const XsSyntaxNode *first = path_identifier_at(path, 0);
  if (first == NULL)
    return true;
  if (path_segment_count(path) == 1 &&
      (primitive_type_name(first->text) || generic_scope_contains(generics, first->text)))
    return true;
  char *name = path_to_string(path);
  if (name == NULL)
    return false;
  const XsHirSymbol *symbol = resolve_user_type(name, namespace_name, symbols, imports);
  bool success = true;
  if (!symbol_is_type(symbol))
    success = report_unresolved_type(diagnostics, type, name);
  else if (!symbol_visible_from(symbol, namespace_name))
    success = report_invisible_type(diagnostics, type, name, namespace_name);
  else {
    if (check_arity) {
      size_t expected = generic_parameter_count(symbol->syntax);
      if (expected != 0)
        success = report_generic_arity(diagnostics, type, name, expected, 0);
    }
    if (resolved != NULL)
      *resolved = symbol;
  }
  free(name);
  return success;
}

static bool resolve_generic_type_node(const XsSyntaxNode *type, const char *namespace_name,
                                      const GenericScope *generics, const XsHirSymbolTable *symbols,
                                      const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  if (type->child_count == 0)
    return true;
  const XsSyntaxNode *base = type->children[0];
  const XsHirSymbol *symbol = NULL;
  bool success = resolve_named_type(base, namespace_name, generics, symbols, imports, diagnostics, false, &symbol);
  if (symbol != NULL) {
    char *name = path_to_string(first_child_kind(base, XS_SYNTAX_PATH));
    if (name == NULL)
      return false;
    size_t expected = generic_parameter_count(symbol->syntax);
    size_t actual = type->child_count - 1;
    if (expected != actual)
      success = report_generic_arity(diagnostics, type, name, expected, actual) && success;
    free(name);
  }
  for (size_t i = 1; i < type->child_count; ++i)
    success = resolve_type_node(type->children[i], namespace_name, generics, symbols, imports, diagnostics) && success;
  return success;
}

static bool resolve_constraint_type(const XsSyntaxNode *type, const char *namespace_name, const GenericScope *generics,
                                    const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                                    XsDiagnostics *diagnostics)
{
  if (type->kind != XS_SYNTAX_TYPE_NAMED) {
    return resolve_type_node(type, namespace_name, generics, symbols, imports, diagnostics);
  }
  const XsSyntaxNode *path = first_child_kind(type, XS_SYNTAX_PATH);
  char *name = path_to_string(path);
  if (name == NULL)
    return false;
  const XsHirSymbol *symbol = NULL;
  bool success = resolve_named_type(type, namespace_name, generics, symbols, imports, diagnostics, true, &symbol);
  if (symbol != NULL && symbol->kind != XS_HIR_SYMBOL_INTERFACE)
    success = report_constraint_kind(diagnostics, type, name) && success;
  free(name);
  return success;
}

static bool resolve_generic_parameter_constraints(const XsSyntaxNode *parameter, const char *namespace_name,
                                                  const GenericScope *generics, const XsHirSymbolTable *symbols,
                                                  const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  bool success = true;
  for (size_t i = 0; i < parameter->child_count; ++i) {
    const XsSyntaxNode *child = parameter->children[i];
    if (child->kind == XS_SYNTAX_IDENTIFIER)
      continue;
    success = resolve_constraint_type(child, namespace_name, generics, symbols, imports, diagnostics) && success;
  }
  return success;
}

static bool resolve_type_node(const XsSyntaxNode *type, const char *namespace_name, const GenericScope *generics,
                              const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                              XsDiagnostics *diagnostics)
{
  if (type == NULL)
    return true;
  bool success = true;
  if (type->kind == XS_SYNTAX_TYPE_NAMED) {
    const XsHirSymbol *symbol = NULL;
    return resolve_named_type(type, namespace_name, generics, symbols, imports, diagnostics, true, &symbol);
  }
  if (type->kind == XS_SYNTAX_TYPE_GENERIC)
    return resolve_generic_type_node(type, namespace_name, generics, symbols, imports, diagnostics);
  for (size_t i = 0; i < type->child_count; ++i) {
    if (type->children[i]->kind == XS_SYNTAX_EXPR_LITERAL)
      continue;
    success = resolve_type_node(type->children[i], namespace_name, generics, symbols, imports, diagnostics) && success;
  }
  return success;
}

static bool declaration_opens_generic_scope(XsSyntaxKind kind)
{
  return kind == XS_SYNTAX_DECL_FUNCTION || kind == XS_SYNTAX_DECL_CLASS || kind == XS_SYNTAX_DECL_INTERFACE ||
         kind == XS_SYNTAX_DECL_ENUM || kind == XS_SYNTAX_DECL_DATA;
}

static bool resolve_declaration_types(const XsSyntaxNode *node, const char *namespace_name,
                                      const GenericScope *generics, const XsHirSymbolTable *symbols,
                                      const XsHirImportScope *imports, XsDiagnostics *diagnostics)
{
  GenericScope local_scope = {.parent = generics, .declaration = node};
  const GenericScope *active = declaration_opens_generic_scope(node->kind) ? &local_scope : generics;
  bool success = true;
  if (node->kind == XS_SYNTAX_GENERIC_PARAMETER)
    return resolve_generic_parameter_constraints(node, namespace_name, active, symbols, imports, diagnostics);
  if (node->kind == XS_SYNTAX_TYPE_NAMED || node->kind == XS_SYNTAX_TYPE_GENERIC ||
      node->kind == XS_SYNTAX_TYPE_ARRAY || node->kind == XS_SYNTAX_TYPE_FIXED_ARRAY ||
      node->kind == XS_SYNTAX_TYPE_POINTER || node->kind == XS_SYNTAX_TYPE_REFERENCE ||
      node->kind == XS_SYNTAX_TYPE_MUTABLE_REFERENCE || node->kind == XS_SYNTAX_TYPE_TUPLE ||
      node->kind == XS_SYNTAX_TYPE_FUNCTION)
    return resolve_type_node(node, namespace_name, active, symbols, imports, diagnostics);
  for (size_t i = 0; i < node->child_count; ++i)
    success =
        resolve_declaration_types(node->children[i], namespace_name, active, symbols, imports, diagnostics) && success;
  return success;
}

bool xs_hir_resolve_types(const XsSyntaxTree *tree, const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                          XsDiagnostics *diagnostics)
{
  if (tree == NULL || tree->root == NULL || symbols == NULL || imports == NULL || diagnostics == NULL)
    return false;
  bool success = true;
  for (size_t i = 0; i < symbols->count; ++i) {
    const XsHirSymbol *symbol = &symbols->symbols[i];
    if (symbol->span.file_id != tree->file_id)
      continue;
    success = resolve_declaration_types(symbol->syntax, symbol->namespace_name, NULL, symbols, imports, diagnostics) &&
              success;
  }
  return success && !xs_diagnostics_has_error(diagnostics);
}
