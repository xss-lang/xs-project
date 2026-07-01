#include "xs/diagnostic.h"
#include "xs/macro.h"
#include "xs/syntax_ast.h"
#include "xs/syntax_parser.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while (0)

static bool text_is(XsText text, const char *value)
{
  return text.length == strlen(value) && memcmp(text.data, value, text.length) == 0;
}

static const XsSyntaxNode *first_identifier(const XsSyntaxNode *node)
{
  return xs_syntax_find_first(node, XS_SYNTAX_IDENTIFIER);
}

static void test_top_level_declaration_view_expands_macro_calls(void)
{
  const char *text = "module App;\n"
                     "macroRules! make {\n"
                     "  (): { incomplete fn First(); };\n"
                     "  (): { incomplete fn Second(); };\n"
                     "}\n"
                     "make!();\n"
                     "data Tail { value: int }\n";
  XsSource source = {.path = "ExpandedView.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroDeclarationExpansionSet declarations;
  XsMacroExpandedDeclarationSet expanded;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 91, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(declarations.count == 2);
  CHECK(xs_macro_expand_top_level_declarations(&tree, &declarations, &diagnostics, &expanded));
  CHECK(expanded.count == 5);
  CHECK(expanded.count < 3 || expanded.items[2].from_macro_expansion);
  CHECK(expanded.count < 4 || expanded.items[3].from_macro_expansion);
  CHECK(expanded.count < 3 || expanded.items[2].declaration->kind == XS_SYNTAX_DECL_FUNCTION);
  CHECK(expanded.count < 4 || expanded.items[3].declaration->kind == XS_SYNTAX_DECL_FUNCTION);
  CHECK(expanded.count < 5 || expanded.items[4].declaration->kind == XS_SYNTAX_DECL_DATA);
  const XsSyntaxNode *first = expanded.count < 3 ? nullptr : first_identifier(expanded.items[2].declaration);
  const XsSyntaxNode *second = expanded.count < 4 ? nullptr : first_identifier(expanded.items[3].declaration);
  CHECK(first != nullptr && text_is(first->text, "First"));
  CHECK(second != nullptr && text_is(second->text, "Second"));
  xs_macro_expanded_declaration_set_free(&expanded);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_child_declaration_view_expands_member_macro_calls(void)
{
  const char *text = "module App;\n"
                     "class User {\n"
                     "  macroRules! make { (): { incomplete fn Generated(); }; }\n"
                     "  make!();\n"
                     "}\n";
  XsSource source = {.path = "ExpandedMemberView.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroDeclarationExpansionSet declarations;
  XsMacroExpandedDeclarationSet expanded;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 92, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(declarations.count == 1);
  const XsSyntaxNode *class_node = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_CLASS);
  CHECK(class_node != nullptr);
  CHECK(xs_macro_expand_child_declarations(class_node, &declarations, &diagnostics, &expanded));
  const XsSyntaxNode *generated = nullptr;
  for (size_t i = 0; i < expanded.count; ++i) {
    if (expanded.items[i].from_macro_expansion && expanded.items[i].declaration->kind == XS_SYNTAX_DECL_FUNCTION)
      generated = expanded.items[i].declaration;
  }
  CHECK(generated != nullptr);
  const XsSyntaxNode *name = first_identifier(generated);
  CHECK(name != nullptr && text_is(name->text, "Generated"));
  xs_macro_expanded_declaration_set_free(&expanded);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_child_declaration_view_expands_field_like_macro_calls(void)
{
  const char *text = "module App;\n"
                     "class User {\n"
                     "  macroRules! make { (): { value: int; }; }\n"
                     "  make!();\n"
                     "}\n";
  XsSource source = {.path = "ExpandedFieldLikeMemberView.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroDeclarationExpansionSet declarations;
  XsMacroExpandedDeclarationSet expanded;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 93, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(declarations.count == 1);
  const XsSyntaxNode *class_node = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_CLASS);
  CHECK(class_node != nullptr);
  CHECK(xs_macro_expand_child_declarations(class_node, &declarations, &diagnostics, &expanded));
  const XsSyntaxNode *field_like = nullptr;
  for (size_t i = 0; i < expanded.count; ++i) {
    if (expanded.items[i].from_macro_expansion && expanded.items[i].declaration->kind == XS_SYNTAX_DECL_VARIABLE)
      field_like = expanded.items[i].declaration;
  }
  CHECK(field_like != nullptr);
  const XsSyntaxNode *name = first_identifier(field_like);
  CHECK(name != nullptr && text_is(name->text, "value"));
  xs_macro_expanded_declaration_set_free(&expanded);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_child_statement_view_expands_macro_calls(void)
{
  const char *text = "module App;\n"
                     "macroRules! emit { (): { Known(); }; }\n"
                     "fn Known() {}\n"
                     "fn Main() {\n"
                     "  Before();\n"
                     "  emit!();\n"
                     "  After();\n"
                     "}\n";
  XsSource source = {.path = "ExpandedStatementView.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroStatementExpansionSet statements;
  XsMacroExpandedStatementSet expanded;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 94, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  const XsSyntaxNode *function = nullptr;
  for (size_t i = 0; tree.root != nullptr && i < tree.root->child_count; ++i) {
    const XsSyntaxNode *child = tree.root->children[i];
    const XsSyntaxNode *name = child->kind == XS_SYNTAX_DECL_FUNCTION ? first_identifier(child) : nullptr;
    if (name != nullptr && text_is(name->text, "Main"))
      function = child;
  }
  const XsSyntaxNode *block = xs_syntax_find_first(function, XS_SYNTAX_STMT_BLOCK);
  CHECK(block != nullptr);
  CHECK(xs_macro_expand_child_statements(block, &statements, &diagnostics, &expanded));
  CHECK(expanded.count == 3);
  CHECK(expanded.count < 2 || expanded.items[1].from_macro_expansion);
  CHECK(expanded.count < 1 || expanded.items[0].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(expanded.count < 2 || expanded.items[1].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(expanded.count < 3 || expanded.items[2].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  const XsSyntaxNode *replacement = expanded.count < 2 ? nullptr : first_identifier(expanded.items[1].statement);
  CHECK(replacement != nullptr && text_is(replacement->text, "Known"));
  xs_macro_expanded_statement_set_free(&expanded);
  xs_macro_statement_expansion_set_free(&statements);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_top_level_declaration_view_expands_macro_calls();
  test_child_declaration_view_expands_member_macro_calls();
  test_child_declaration_view_expands_field_like_macro_calls();
  test_child_statement_view_expands_macro_calls();
  return failures == 0 ? 0 : 1;
}
