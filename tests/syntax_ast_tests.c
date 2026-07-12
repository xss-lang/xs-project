/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/diagnostic.h"
#include "xs/macro.h"
#include "xs/syntax_ast.h"
#include "xs/syntax_parser.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while(0)

static size_t count_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if(node == nullptr)
    return 0;
  size_t count = node->kind == kind ? 1 : 0;
  for(size_t index = 0; index < node->child_count; ++index)
    count += count_kind(node->children[index], kind);
  return count;
}

static void test_function_tree(void)
{
  const char *text = "fn Add(a: Int, b: Int) => Int {\n    result: Int = a + b;\n    return result;\n}\n";
  XsSource source = {.path = "Add.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 42, &diagnostics, &tree));
  CHECK(tree.root != nullptr && tree.root->kind == XS_SYNTAX_FILE);
  if(tree.root == nullptr)
  {
    xs_syntax_tree_free(&tree);
    xs_diagnostics_free(&diagnostics);
    return;
  }
  CHECK(tree.root->child_count == 1);
  const XsSyntaxNode *function = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_FUNCTION);
  CHECK(function != nullptr);
  if(function == nullptr)
  {
    xs_syntax_tree_free(&tree);
    xs_diagnostics_free(&diagnostics);
    return;
  }
  CHECK(function->span.file_id == 42);
  CHECK(function->span.start_line == 1);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_PARAMETER) != nullptr);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_TYPE_NAMED) != nullptr);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_STMT_VARIABLE) != nullptr);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_EXPR_BINARY) != nullptr);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_STMT_RETURN) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_module_import_and_macro(void)
{
  const char *text = "module App.Core;\nimports Math.Advanced;\n"
                     "macroRules! identity { ($value:expr): { $value }; }\n"
                     "fn Main() { identity!(42); }\n";
  XsSource source = {.path = "Main.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 7, &diagnostics, &tree));
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_MODULE) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_IMPORT) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_MACRO) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_MATCHER_FRAGMENT) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_EXPANSION_VARIABLE) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_MACRO_CALL) != nullptr);
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_macro_call_declaration_structure(void)
{
  const char *text = "macroRules! createItem { (): {}; }\n"
                     "createItem!();\n"
                     "class Host { createItem!(); }\n";
  XsSource source = {.path = "MacroDeclarationCall.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 8, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_DECL_MACRO_CALL) == 2);
  const XsSyntaxNode *call = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_MACRO_CALL);
  CHECK(call != nullptr);
  CHECK(call == nullptr || xs_syntax_find_first(call, XS_SYNTAX_EXPR_MACRO_CALL) != nullptr);
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_control_flow_structure(void)
{
  const char *text = "fn Flow(value: Int) {\n"
                     "  for (i: Int = 0; i < 3; i = i + 1) { continue; }\n"
                     "  while (value > 0) { break; }\n"
                     "  match (value) { 0 -> { return; }, else -> { throw IOException(\"x\"); }, }\n"
                     "  try {} catch (error: IOException) {} finally {}\n"
                     "}\n";
  XsSource source = {.path = "Flow.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 9, &diagnostics, &tree));
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_FOR) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_WHILE) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_MATCH) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_TRY) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_CATCH) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_control_flow_expression_structure(void)
{
  const char *text = "fn Choose(value: Int) => Int {\n"
                     "  selected: Int = if (value > 0) { 1; } else { 0; };\n"
                     "  return match (selected) { 0 -> { 10; }, else -> { 20; }, };\n"
                     "}\n";
  XsSource source = {.path = "ControlFlowExpressions.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 28, &diagnostics, &tree));
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_IF) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_MATCH) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_if_expression_requires_else(void)
{
  const char *text = "fn Choose(value: Int) => Int {\n"
                     "  return if (value > 0) { 1; };\n"
                     "}\n";
  XsSource source = {.path = "InvalidIfExpression.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 29, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_function_expression_structure(void)
{
  const char *text = "fn Spawn() {\n"
                     "  Thread.spawn(move fn() => Int { return 42; });\n"
                     "  mapper: Mapper = fn(value: Int) => Int { return value + 1; };\n"
                     "}\n";
  XsSource source = {.path = "ThreadClosure.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 17, &diagnostics, &tree));
  const XsSyntaxNode *function_expression = xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_FUNCTION);
  CHECK(function_expression != nullptr);
  CHECK(function_expression == nullptr || (function_expression->flags & XS_SYNTAX_FLAG_MOVE_CAPTURE) != 0);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_FUNCTION) == 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_PARAMETER) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_NAMED) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_RETURN) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_new_expression_structure(void)
{
  const char *text = "fn Main() {\n"
                     "  user: User = new();\n"
                     "  client: Http.client = new();\n"
                     "}\n";
  XsSource source = {.path = "NewExpression.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 18, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_NEW) == 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_VARIABLE) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_else_discard_statement_structure(void)
{
  const char *text = "fn Main() { else: a = new(); if (true) {} else: b = new(); }\n";
  XsSource source = {.path = "Discard.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 30, &diagnostics, &tree));
  const XsSyntaxNode *discard = xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_DISCARD);
  CHECK(discard != nullptr);
  CHECK(discard == nullptr || discard->child_count == 1);
  CHECK(discard == nullptr || discard->children[0]->kind == XS_SYNTAX_EXPR_ASSIGNMENT);
  CHECK(count_kind(tree.root, XS_SYNTAX_STMT_DISCARD) == 2);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_NEW) == 2);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_top_level_execution_stays_invalid(void)
{
  const char *text = "Run();\n";
  XsSource source = {.path = "TopLevelExecution.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 20, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_data_field_set_and_get_structure(void)
{
  const char *text = "fn Update(user: User) {\n"
                     "  set.name{\"Alfa\"};\n"
                     "  name: Str = user get.name;\n"
                     "}\n";
  XsSource source = {.path = "DataAccess.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 21, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_FIELD_SET) == 1);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_MEMBER_ACCESS) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_function_type_structure(void)
{
  const char *text = "fn Main() {\n"
                     "  mapper: fn(Int, Str) => Bool = fn(value: Int, name: Str) => Bool { return true; };\n"
                     "  done: fn() => () = fn() { return; };\n"
                     "}\n";
  XsSource source = {.path = "FunctionTypes.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 22, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_TYPE_FUNCTION) == 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_UNIT) != nullptr);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_FUNCTION) == 2);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_io_target_expression_structure(void)
{
  const char *text = "fn Io(openedFile: file) {\n"
                     "  std.fout << [stdout] << \"Hello\";\n"
                     "  std.fout << [openedFile] <<< [stderr];\n"
                     "  std.fin >> [stdin] >> value;\n"
                     "}\n";
  XsSource source = {.path = "IoTargets.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 23, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_IO_TARGET) == 4);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_BINARY) >= 4);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_character_literal_structure(void)
{
  const char *text = "fn Main(value: Char) {\n"
                     "  letter: Char = 'A';\n"
                     "  newline: Char = '\\n';\n"
                     "  match (value) { 'A' -> { return; }, else -> { return; }, }\n"
                     "}\n";
  XsSource source = {.path = "CharacterLiteral.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 24, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_LITERAL) >= 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_PATTERN_LITERAL) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_optional_operator_structure(void)
{
  const char *text = "fn Main(user: Optional<User>, fallback: Str) {\n"
                     "  name: Optional<Str> = user?.Name;\n"
                     "  display: Str = name ?? fallback;\n"
                     "  name ?"
                     "?= Some(\"guest\");\n"
                     "  forced: Str = name!;\n"
                     "}\n";
  XsSource source = {.path = "OptionalOperators.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 27, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_OPTIONAL_MEMBER_ACCESS) == 1);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_OPTIONAL_FORGIVING) == 1);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_BINARY) >= 1);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_ASSIGNMENT) >= 1);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_lifetime_type_structure(void)
{
  const char *text = "fn Print(first: &'a User, second: &'b mut User, shared: &'static Str, inferred: &'_ User) {\n"
                     "  return;\n"
                     "}\n";
  XsSource source = {.path = "Lifetimes.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 25, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_LIFETIME) == 4);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_REFERENCE) != nullptr);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_MUTABLE_REFERENCE) != nullptr);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_nested_generic_type_closers(void)
{
  const char *text = "interface Parser<T> { fn Parse(value: T); }\n"
                     "fn UseParser<T: Parser<Box<Int>> >(value: T, items: List<Box<Int>>) {}\n";
  XsSource source = {.path = "NestedGenerics.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 26, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_TYPE_GENERIC) == 4);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_function_tree();
  test_module_import_and_macro();
  test_macro_call_declaration_structure();
  test_control_flow_structure();
  test_control_flow_expression_structure();
  test_if_expression_requires_else();
  test_function_expression_structure();
  test_new_expression_structure();
  test_else_discard_statement_structure();
  test_top_level_execution_stays_invalid();
  test_data_field_set_and_get_structure();
  test_function_type_structure();
  test_io_target_expression_structure();
  test_character_literal_structure();
  test_optional_operator_structure();
  test_lifetime_type_structure();
  test_nested_generic_type_closers();
  return failures == 0 ? 0 : 1;
}
