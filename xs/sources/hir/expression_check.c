#include "xs/hir/expression_check.h"

#include "xs/hir/type_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  XsText name;
  const XsSyntaxNode *declaration;
  bool immutable;
} LocalBinding;

typedef struct LocalScope LocalScope;

struct LocalScope
{
  LocalScope *parent;
  LocalBinding *bindings;
  size_t count;
  size_t capacity;
  bool allocation_failed;
};

static const XsSyntaxNode *first_child_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if (node == nullptr)
    return nullptr;
  for (size_t i = 0; i < node->child_count; ++i) {
    if (node->children[i]->kind == kind)
      return node->children[i];
  }
  return nullptr;
}

static bool text_equal(XsText left, XsText right)
{
  return left.length == right.length && memcmp(left.data, right.data, left.length) == 0;
}

static XsSpan node_span(const XsSyntaxNode *node)
{
  return (XsSpan){.start = node->span.start_offset, .end = node->span.end_offset};
}

static const XsSyntaxNode *single_path_identifier(const XsSyntaxNode *type)
{
  if (type == nullptr || type->kind != XS_SYNTAX_TYPE_NAMED)
    return nullptr;
  const XsSyntaxNode *path = first_child_kind(type, XS_SYNTAX_PATH);
  if (path == nullptr || path->child_count != 1 || path->children[0]->kind != XS_SYNTAX_IDENTIFIER)
    return nullptr;
  return path->children[0];
}

static const XsHirPrimitiveInfo *primitive_from_type(const XsSyntaxNode *type)
{
  const XsSyntaxNode *identifier = single_path_identifier(type);
  if (identifier == nullptr)
    return nullptr;
  return xs_hir_primitive_find(identifier->text.data, identifier->text.length);
}

static const char *literal_kind_name(XsTokenKind kind)
{
  switch (kind) {
  case XS_TOKEN_INTEGER:
    return "integer";
  case XS_TOKEN_FLOAT:
    return "float";
  case XS_TOKEN_STRING:
    return "string";
  case XS_TOKEN_CHARACTER:
    return "char";
  case XS_TOKEN_KW_TRUE:
  case XS_TOKEN_KW_FALSE:
    return "bool";
  default:
    return "literal";
  }
}

static bool literal_matches_primitive(XsTokenKind literal, const XsHirPrimitiveInfo *primitive)
{
  if (primitive == nullptr || literal == XS_TOKEN_KW_NIL)
    return true;
  if (literal == XS_TOKEN_INTEGER)
    return primitive->is_integer && primitive->kind != XS_HIR_PRIMITIVE_BOOL &&
           primitive->kind != XS_HIR_PRIMITIVE_CHAR;
  if (literal == XS_TOKEN_FLOAT)
    return primitive->is_float;
  if (literal == XS_TOKEN_STRING)
    return primitive->kind == XS_HIR_PRIMITIVE_STR;
  if (literal == XS_TOKEN_CHARACTER)
    return primitive->kind == XS_HIR_PRIMITIVE_CHAR;
  if (literal == XS_TOKEN_KW_TRUE || literal == XS_TOKEN_KW_FALSE)
    return primitive->kind == XS_HIR_PRIMITIVE_BOOL;
  return true;
}

static bool report_literal_type_error(XsDiagnostics *diagnostics, const XsSyntaxNode *literal,
                                      const XsHirPrimitiveInfo *primitive)
{
  char message[256];
  snprintf(message, sizeof(message), "%s literal is not assignable to primitive type '%s'",
           literal_kind_name(literal->token_kind), primitive->name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(literal), message);
}

static bool report_immutable_assignment(XsDiagnostics *diagnostics, const XsSyntaxNode *target,
                                        const LocalBinding *binding)
{
  int length = binding->name.length > 128 ? 128 : (int)binding->name.length;
  char message[256];
  snprintf(message, sizeof(message), "cannot assign to immutable local '%.*s'", length, binding->name.data);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(target), message);
}

static bool check_variable_initializer(const XsSyntaxNode *declaration, XsDiagnostics *diagnostics)
{
  if (declaration->child_count < 3)
    return true;
  const XsHirPrimitiveInfo *primitive = primitive_from_type(declaration->children[1]);
  const XsSyntaxNode *initializer = declaration->children[2];
  if (primitive == nullptr || initializer->kind != XS_SYNTAX_EXPR_LITERAL)
    return true;
  if (literal_matches_primitive(initializer->token_kind, primitive))
    return true;
  return report_literal_type_error(diagnostics, initializer, primitive);
}

static const XsSyntaxNode *declaration_identifier(const XsSyntaxNode *declaration)
{
  return declaration->child_count == 0 || declaration->children[0]->kind != XS_SYNTAX_IDENTIFIER
             ? nullptr
             : declaration->children[0];
}

static bool local_scope_add(LocalScope *scope, const XsSyntaxNode *declaration)
{
  const XsSyntaxNode *identifier = declaration_identifier(declaration);
  if (scope == nullptr || identifier == nullptr)
    return true;
  if (scope->count == scope->capacity) {
    size_t capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
    LocalBinding *bindings = realloc(scope->bindings, capacity * sizeof(*bindings));
    if (bindings == nullptr) {
      scope->allocation_failed = true;
      return false;
    }
    scope->bindings = bindings;
    scope->capacity = capacity;
  }
  bool immutable = (declaration->flags & (XS_SYNTAX_FLAG_IMMUTABLE | XS_SYNTAX_FLAG_CONSTANT |
                                          XS_SYNTAX_FLAG_STATIC_CONSTANT)) != 0;
  scope->bindings[scope->count++] =
      (LocalBinding){.name = identifier->text, .declaration = declaration, .immutable = immutable};
  return true;
}

static const LocalBinding *local_scope_find(const LocalScope *scope, XsText name)
{
  for (const LocalScope *current = scope; current != nullptr; current = current->parent) {
    for (size_t i = current->count; i > 0; --i) {
      if (text_equal(current->bindings[i - 1].name, name))
        return &current->bindings[i - 1];
    }
  }
  return nullptr;
}

static void local_scope_free(LocalScope *scope)
{
  free(scope->bindings);
  *scope = (LocalScope){0};
}

static bool has_child_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if (node == nullptr)
    return false;
  for (size_t i = 0; i < node->child_count; ++i) {
    if (node->children[i]->kind == kind)
      return true;
  }
  return false;
}

static const XsSyntaxNode *assignment_identifier_target(const XsSyntaxNode *node)
{
  if (node == nullptr || node->kind != XS_SYNTAX_EXPR_ASSIGNMENT || node->child_count < 1)
    return nullptr;
  const XsSyntaxNode *target = node->children[0];
  if (target->kind != XS_SYNTAX_EXPR_IDENTIFIER || target->child_count == 0 ||
      target->children[0]->kind != XS_SYNTAX_IDENTIFIER)
    return nullptr;
  return target->children[0];
}

static bool check_assignment(const XsSyntaxNode *node, const LocalScope *scope, XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *identifier = assignment_identifier_target(node);
  if (identifier == nullptr)
    return true;
  const LocalBinding *binding = local_scope_find(scope, identifier->text);
  if (binding == nullptr || !binding->immutable)
    return true;
  return report_immutable_assignment(diagnostics, identifier, binding);
}

static bool check_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                       const XsMacroStatementExpansionSet *macro_statements, LocalScope *scope,
                       XsDiagnostics *diagnostics);

static bool check_children(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                           const XsMacroStatementExpansionSet *macro_statements, LocalScope *scope,
                           XsDiagnostics *diagnostics)
{
  bool success = true;
  for (size_t i = 0; i < node->child_count; ++i) {
    success = check_node(node->children[i], macro_declarations, macro_statements, scope, diagnostics) && success;
  }
  return success;
}

static bool check_expanded_declaration_children(const XsSyntaxNode *node,
                                                const XsMacroDeclarationExpansionSet *macro_declarations,
                                                const XsMacroStatementExpansionSet *macro_statements,
                                                LocalScope *scope, XsDiagnostics *diagnostics)
{
  XsMacroExpandedDeclarationSet expanded = {0};
  if (!xs_macro_expand_child_declarations(node, macro_declarations, diagnostics, &expanded))
    return false;
  bool success = true;
  for (size_t i = 0; i < expanded.count; ++i)
    success = check_node(expanded.items[i].declaration, macro_declarations, macro_statements, scope, diagnostics) &&
              success;
  xs_macro_expanded_declaration_set_free(&expanded);
  return success;
}

static bool check_expanded_statement_children(const XsSyntaxNode *node,
                                              const XsMacroDeclarationExpansionSet *macro_declarations,
                                              const XsMacroStatementExpansionSet *macro_statements,
                                              LocalScope *scope, XsDiagnostics *diagnostics)
{
  XsMacroExpandedStatementSet expanded = {0};
  if (!xs_macro_expand_child_statements(node, macro_statements, diagnostics, &expanded))
    return false;
  bool success = true;
  for (size_t i = 0; i < expanded.count; ++i)
    success = check_node(expanded.items[i].statement, macro_declarations, macro_statements, scope, diagnostics) &&
              success;
  xs_macro_expanded_statement_set_free(&expanded);
  return success;
}

static bool check_scoped_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                              const XsMacroStatementExpansionSet *macro_statements, LocalScope *parent,
                              XsDiagnostics *diagnostics)
{
  LocalScope scope = {.parent = parent};
  bool success = true;
  if (macro_declarations != nullptr && has_child_kind(node, XS_SYNTAX_DECL_MACRO_CALL))
    success = check_expanded_declaration_children(node, macro_declarations, macro_statements, &scope, diagnostics);
  else if (macro_statements != nullptr && has_child_kind(node, XS_SYNTAX_STMT_MACRO_CALL))
    success = check_expanded_statement_children(node, macro_declarations, macro_statements, &scope, diagnostics);
  else
    success = check_children(node, macro_declarations, macro_statements, &scope, diagnostics);
  bool allocation_failed = scope.allocation_failed;
  local_scope_free(&scope);
  return success && !allocation_failed;
}

static bool check_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                       const XsMacroStatementExpansionSet *macro_statements, LocalScope *scope,
                       XsDiagnostics *diagnostics)
{
  if (node == nullptr)
    return true;
  bool success = true;
  if (node->kind == XS_SYNTAX_DECL_FUNCTION || node->kind == XS_SYNTAX_EXPR_FUNCTION ||
      node->kind == XS_SYNTAX_STMT_BLOCK)
    return check_scoped_node(node, macro_declarations, macro_statements, scope, diagnostics);
  if (node->kind == XS_SYNTAX_DECL_VARIABLE) {
    success = check_variable_initializer(node, diagnostics) && success;
    success = local_scope_add(scope, node) && success;
  }
  if (node->kind == XS_SYNTAX_EXPR_ASSIGNMENT)
    success = check_assignment(node, scope, diagnostics) && success;
  if (macro_declarations != nullptr && has_child_kind(node, XS_SYNTAX_DECL_MACRO_CALL))
    return check_expanded_declaration_children(node, macro_declarations, macro_statements, scope, diagnostics) &&
           success;
  if (macro_statements != nullptr && has_child_kind(node, XS_SYNTAX_STMT_MACRO_CALL))
    return check_expanded_statement_children(node, macro_declarations, macro_statements, scope, diagnostics) &&
           success;
  return check_children(node, macro_declarations, macro_statements, scope, diagnostics) && success;
}

bool xs_hir_check_expression_types_with_macros(const XsSyntaxTree *tree,
                                               const XsMacroDeclarationExpansionSet *macro_declarations,
                                               const XsMacroStatementExpansionSet *macro_statements,
                                               XsDiagnostics *diagnostics)
{
  if (tree == nullptr || tree->root == nullptr || diagnostics == nullptr)
    return false;
  return check_node(tree->root, macro_declarations, macro_statements, nullptr, diagnostics) &&
         !xs_diagnostics_has_error(diagnostics);
}

bool xs_hir_check_expression_types(const XsSyntaxTree *tree, XsDiagnostics *diagnostics)
{
  return xs_hir_check_expression_types_with_macros(tree, nullptr, nullptr, diagnostics);
}
