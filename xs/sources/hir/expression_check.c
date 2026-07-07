/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

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
typedef struct CheckContext CheckContext;

struct LocalScope
{
  LocalScope *parent;
  LocalBinding *bindings;
  size_t count;
  size_t capacity;
  bool allocation_failed;
};

struct CheckContext
{
  LocalScope *scope;
  const XsHirPrimitiveInfo *return_type;
};

static const XsSyntaxNode *first_child_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if (node == nullptr)
    return nullptr;
  for (size_t i = 0; i < node->child_count; ++i)
  {
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
  switch (kind)
  {
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

static bool report_assignment_literal_type_error(XsDiagnostics *diagnostics, const XsSyntaxNode *literal,
                                                 const XsHirPrimitiveInfo *primitive, const LocalBinding *binding)
{
  int length = binding->name.length > 128 ? 128 : (int)binding->name.length;
  char message[320];
  snprintf(message, sizeof(message), "%s literal is not assignable to local '%.*s' of primitive type '%s'",
           literal_kind_name(literal->token_kind), length, binding->name.data, primitive->name);
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

static bool report_return_literal_type_error(XsDiagnostics *diagnostics, const XsSyntaxNode *literal,
                                             const XsHirPrimitiveInfo *primitive)
{
  char message[256];
  snprintf(message, sizeof(message), "%s literal is not assignable to function return type '%s'",
           literal_kind_name(literal->token_kind), primitive->name);
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(literal), message);
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
  if (scope->count == scope->capacity)
  {
    size_t capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
    LocalBinding *bindings = realloc(scope->bindings, capacity * sizeof(*bindings));
    if (bindings == nullptr)
    {
      scope->allocation_failed = true;
      return false;
    }
    scope->bindings = bindings;
    scope->capacity = capacity;
  }
  bool immutable =
      (declaration->flags & (XS_SYNTAX_FLAG_IMMUTABLE | XS_SYNTAX_FLAG_CONSTANT | XS_SYNTAX_FLAG_STATIC_CONSTANT)) != 0;
  scope->bindings[scope->count++] =
      (LocalBinding){.name = identifier->text, .declaration = declaration, .immutable = immutable};
  return true;
}

static const LocalBinding *local_scope_find(const LocalScope *scope, XsText name)
{
  for (const LocalScope *current = scope; current != nullptr; current = current->parent)
  {
    for (size_t i = current->count; i > 0; --i)
    {
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
  for (size_t i = 0; i < node->child_count; ++i)
  {
    if (node->children[i]->kind == kind)
      return true;
  }
  return false;
}

static const XsHirPrimitiveInfo *function_return_primitive(const XsSyntaxNode *function)
{
  if (function == nullptr || (function->kind != XS_SYNTAX_DECL_FUNCTION && function->kind != XS_SYNTAX_EXPR_FUNCTION))
    return nullptr;
  for (size_t i = 0; i < function->child_count; ++i)
  {
    const XsSyntaxNode *child = function->children[i];
    if (child->kind == XS_SYNTAX_TYPE_NAMED && (child->flags & XS_SYNTAX_FLAG_RETURN_TYPE) != 0)
      return primitive_from_type(child);
  }
  return nullptr;
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

static const XsSyntaxNode *assignment_value(const XsSyntaxNode *node)
{
  return node != nullptr && node->kind == XS_SYNTAX_EXPR_ASSIGNMENT && node->child_count >= 3 ? node->children[2]
                                                                                              : nullptr;
}

static bool check_return_statement(const XsSyntaxNode *node, const CheckContext *context, XsDiagnostics *diagnostics)
{
  if (node == nullptr || node->kind != XS_SYNTAX_STMT_RETURN || node->child_count == 0 ||
      context->return_type == nullptr)
    return true;
  const XsSyntaxNode *value = node->children[0];
  if (value->kind != XS_SYNTAX_EXPR_LITERAL || literal_matches_primitive(value->token_kind, context->return_type))
    return true;
  return report_return_literal_type_error(diagnostics, value, context->return_type);
}

static bool check_assignment(const XsSyntaxNode *node, const CheckContext *context, XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *identifier = assignment_identifier_target(node);
  if (identifier == nullptr)
    return true;
  const LocalBinding *binding = local_scope_find(context->scope, identifier->text);
  if (binding == nullptr)
    return true;
  bool success = true;
  const XsHirPrimitiveInfo *primitive =
      binding->declaration->child_count >= 2 ? primitive_from_type(binding->declaration->children[1]) : nullptr;
  const XsSyntaxNode *value = assignment_value(node);
  if (primitive != nullptr && value != nullptr && value->kind == XS_SYNTAX_EXPR_LITERAL &&
      !literal_matches_primitive(value->token_kind, primitive))
    success = report_assignment_literal_type_error(diagnostics, value, primitive, binding) && success;
  if (binding->immutable)
    success = report_immutable_assignment(diagnostics, identifier, binding) && success;
  return success;
}

static bool check_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                       const XsMacroStatementExpansionSet *macro_statements, CheckContext context,
                       XsDiagnostics *diagnostics);

static bool check_children(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                           const XsMacroStatementExpansionSet *macro_statements, CheckContext context,
                           XsDiagnostics *diagnostics)
{
  bool success = true;
  for (size_t i = 0; i < node->child_count; ++i)
  {
    success = check_node(node->children[i], macro_declarations, macro_statements, context, diagnostics) && success;
  }
  return success;
}

static bool check_expanded_declaration_children(const XsSyntaxNode *node,
                                                const XsMacroDeclarationExpansionSet *macro_declarations,
                                                const XsMacroStatementExpansionSet *macro_statements,
                                                CheckContext context, XsDiagnostics *diagnostics)
{
  XsMacroExpandedDeclarationSet expanded = {0};
  if (!xs_macro_expand_child_declarations(node, macro_declarations, diagnostics, &expanded))
    return false;
  bool success = true;
  for (size_t i = 0; i < expanded.count; ++i)
    success = check_node(expanded.items[i].declaration, macro_declarations, macro_statements, context, diagnostics) &&
              success;
  xs_macro_expanded_declaration_set_free(&expanded);
  return success;
}

static bool check_expanded_statement_children(const XsSyntaxNode *node,
                                              const XsMacroDeclarationExpansionSet *macro_declarations,
                                              const XsMacroStatementExpansionSet *macro_statements,
                                              CheckContext context, XsDiagnostics *diagnostics)
{
  XsMacroExpandedStatementSet expanded = {0};
  if (!xs_macro_expand_child_statements(node, macro_statements, diagnostics, &expanded))
    return false;
  bool success = true;
  for (size_t i = 0; i < expanded.count; ++i)
    success =
        check_node(expanded.items[i].statement, macro_declarations, macro_statements, context, diagnostics) && success;
  xs_macro_expanded_statement_set_free(&expanded);
  return success;
}

static bool check_scoped_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                              const XsMacroStatementExpansionSet *macro_statements, CheckContext parent,
                              XsDiagnostics *diagnostics)
{
  LocalScope scope = {.parent = parent.scope};
  const XsHirPrimitiveInfo *return_type = function_return_primitive(node);
  CheckContext context = {
      .scope = &scope,
      .return_type = return_type == nullptr ? parent.return_type : return_type,
  };
  bool success = true;
  if (macro_declarations != nullptr && has_child_kind(node, XS_SYNTAX_DECL_MACRO_CALL))
    success = check_expanded_declaration_children(node, macro_declarations, macro_statements, context, diagnostics);
  else if (macro_statements != nullptr && has_child_kind(node, XS_SYNTAX_STMT_MACRO_CALL))
    success = check_expanded_statement_children(node, macro_declarations, macro_statements, context, diagnostics);
  else
    success = check_children(node, macro_declarations, macro_statements, context, diagnostics);
  bool allocation_failed = scope.allocation_failed;
  local_scope_free(&scope);
  return success && !allocation_failed;
}

static bool check_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                       const XsMacroStatementExpansionSet *macro_statements, CheckContext context,
                       XsDiagnostics *diagnostics)
{
  if (node == nullptr)
    return true;
  bool success = true;
  if (node->kind == XS_SYNTAX_DECL_FUNCTION || node->kind == XS_SYNTAX_EXPR_FUNCTION ||
      node->kind == XS_SYNTAX_STMT_BLOCK)
    return check_scoped_node(node, macro_declarations, macro_statements, context, diagnostics);
  if (node->kind == XS_SYNTAX_DECL_VARIABLE)
  {
    success = check_variable_initializer(node, diagnostics) && success;
    success = local_scope_add(context.scope, node) && success;
  }
  if (node->kind == XS_SYNTAX_EXPR_ASSIGNMENT)
    success = check_assignment(node, &context, diagnostics) && success;
  if (node->kind == XS_SYNTAX_STMT_RETURN)
    success = check_return_statement(node, &context, diagnostics) && success;
  if (macro_declarations != nullptr && has_child_kind(node, XS_SYNTAX_DECL_MACRO_CALL))
    return check_expanded_declaration_children(node, macro_declarations, macro_statements, context, diagnostics) &&
           success;
  if (macro_statements != nullptr && has_child_kind(node, XS_SYNTAX_STMT_MACRO_CALL))
    return check_expanded_statement_children(node, macro_declarations, macro_statements, context, diagnostics) &&
           success;
  return check_children(node, macro_declarations, macro_statements, context, diagnostics) && success;
}

bool xs_hir_check_expression_types_with_macros(const XsSyntaxTree *tree,
                                               const XsMacroDeclarationExpansionSet *macro_declarations,
                                               const XsMacroStatementExpansionSet *macro_statements,
                                               XsDiagnostics *diagnostics)
{
  if (tree == nullptr || tree->root == nullptr || diagnostics == nullptr)
    return false;
  return check_node(tree->root, macro_declarations, macro_statements, (CheckContext){0}, diagnostics) &&
         !xs_diagnostics_has_error(diagnostics);
}

bool xs_hir_check_expression_types(const XsSyntaxTree *tree, XsDiagnostics *diagnostics)
{
  return xs_hir_check_expression_types_with_macros(tree, nullptr, nullptr, diagnostics);
}
