#include "xs/hir/expression_check.h"

#include "xs/hir/type_info.h"

#include <stdio.h>

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

static bool check_node(const XsSyntaxNode *node, const XsMacroDeclarationExpansionSet *macro_declarations,
                       const XsMacroStatementExpansionSet *macro_statements, XsDiagnostics *diagnostics)
{
  if (node == nullptr)
    return true;
  bool success = true;
  if (node->kind == XS_SYNTAX_DECL_VARIABLE)
    success = check_variable_initializer(node, diagnostics);
  if (macro_declarations != nullptr && has_child_kind(node, XS_SYNTAX_DECL_MACRO_CALL)) {
    XsMacroExpandedDeclarationSet expanded = {0};
    if (!xs_macro_expand_child_declarations(node, macro_declarations, diagnostics, &expanded))
      return false;
    for (size_t i = 0; i < expanded.count; ++i)
      success = check_node(expanded.items[i].declaration, macro_declarations, macro_statements, diagnostics) && success;
    xs_macro_expanded_declaration_set_free(&expanded);
    return success;
  }
  if (macro_statements != nullptr && has_child_kind(node, XS_SYNTAX_STMT_MACRO_CALL)) {
    XsMacroExpandedStatementSet expanded = {0};
    if (!xs_macro_expand_child_statements(node, macro_statements, diagnostics, &expanded))
      return false;
    for (size_t i = 0; i < expanded.count; ++i)
      success = check_node(expanded.items[i].statement, macro_declarations, macro_statements, diagnostics) && success;
    xs_macro_expanded_statement_set_free(&expanded);
    return success;
  }
  for (size_t i = 0; i < node->child_count; ++i)
    success = check_node(node->children[i], macro_declarations, macro_statements, diagnostics) && success;
  return success;
}

bool xs_hir_check_expression_types_with_macros(const XsSyntaxTree *tree,
                                               const XsMacroDeclarationExpansionSet *macro_declarations,
                                               const XsMacroStatementExpansionSet *macro_statements,
                                               XsDiagnostics *diagnostics)
{
  if (tree == nullptr || tree->root == nullptr || diagnostics == nullptr)
    return false;
  return check_node(tree->root, macro_declarations, macro_statements, diagnostics) &&
         !xs_diagnostics_has_error(diagnostics);
}

bool xs_hir_check_expression_types(const XsSyntaxTree *tree, XsDiagnostics *diagnostics)
{
  return xs_hir_check_expression_types_with_macros(tree, nullptr, nullptr, diagnostics);
}
