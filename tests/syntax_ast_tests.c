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

static size_t count_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if (node == NULL)
    return 0;
  size_t count = node->kind == kind ? 1 : 0;
  for (size_t index = 0; index < node->child_count; ++index)
    count += count_kind(node->children[index], kind);
  return count;
}

static void test_function_tree(void)
{
  const char *text = "fn Add(a: int, b: int) => int {\n    result: int = a + b;\n    return result;\n}\n";
  XsSource source = {.path = "Add.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 42, &diagnostics, &tree));
  CHECK(tree.root != NULL && tree.root->kind == XS_SYNTAX_FILE);
  if (tree.root == NULL) {
    xs_syntax_tree_free(&tree);
    xs_diagnostics_free(&diagnostics);
    return;
  }
  CHECK(tree.root->child_count == 1);
  const XsSyntaxNode *function = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_FUNCTION);
  CHECK(function != NULL);
  if (function == NULL) {
    xs_syntax_tree_free(&tree);
    xs_diagnostics_free(&diagnostics);
    return;
  }
  CHECK(function->span.file_id == 42);
  CHECK(function->span.start_line == 1);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_PARAMETER) != NULL);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_TYPE_NAMED) != NULL);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_STMT_VARIABLE) != NULL);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_EXPR_BINARY) != NULL);
  CHECK(xs_syntax_find_first(function, XS_SYNTAX_STMT_RETURN) != NULL);
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
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_MODULE) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_IMPORT) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_MACRO) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_MATCHER_FRAGMENT) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_EXPANSION_VARIABLE) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_MACRO_CALL) != NULL);
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_control_flow_structure(void)
{
  const char *text = "fn Flow(value: int) {\n"
                     "  for (i: int = 0; i < 3; i = i + 1) { continue; }\n"
                     "  while (value > 0) { break; }\n"
                     "  match value { 0 -> { return; }, else -> { throw IOException(\"x\"); }, }\n"
                     "  try {} catch (error: IOException) {} finally {}\n"
                     "}\n";
  XsSource source = {.path = "Flow.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 9, &diagnostics, &tree));
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_FOR) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_WHILE) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_MATCH) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_TRY) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_CATCH) != NULL);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_function_expression_structure(void)
{
  const char *text = "fn Spawn() {\n"
                     "  Thread.spawn(move fn() => int { return 42; });\n"
                     "  mapper: Mapper = fn(value: int) => int { return value + 1; };\n"
                     "}\n";
  XsSource source = {.path = "ThreadClosure.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 17, &diagnostics, &tree));
  const XsSyntaxNode *function_expression = xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_FUNCTION);
  CHECK(function_expression != NULL);
  CHECK(function_expression == NULL || (function_expression->flags & XS_SYNTAX_FLAG_MOVE_CAPTURE) != 0);
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_FUNCTION) == 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_PARAMETER) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_NAMED) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_RETURN) != NULL);
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
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_VARIABLE) != NULL);
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
                     "  name: str = user get.name;\n"
                     "}\n";
  XsSource source = {.path = "DataAccess.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 21, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_FIELD_SET) == 1);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_MEMBER_ACCESS) != NULL);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_function_type_structure(void)
{
  const char *text = "fn Main() {\n"
                     "  mapper: fn(int, str) => bool = fn(value: int, name: str) => bool { return true; };\n"
                     "  done: fn() => () = fn() { return; };\n"
                     "}\n";
  XsSource source = {.path = "FunctionTypes.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 22, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_TYPE_FUNCTION) == 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_UNIT) != NULL);
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
  const char *text = "fn Main(value: char) {\n"
                     "  letter: char = 'A';\n"
                     "  newline: char = '\\n';\n"
                     "  match value { 'A' -> { return; }, else -> { return; }, }\n"
                     "}\n";
  XsSource source = {.path = "CharacterLiteral.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 24, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_EXPR_LITERAL) >= 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_PATTERN_LITERAL) != NULL);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_lifetime_type_structure(void)
{
  const char *text = "fn Print(first: &'a User, second: &'b mut User, shared: &'static str, inferred: &'_ User) {\n"
                     "  return;\n"
                     "}\n";
  XsSource source = {.path = "Lifetimes.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 25, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_LIFETIME) == 4);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_REFERENCE) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_TYPE_MUTABLE_REFERENCE) != NULL);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_function_tree();
  test_module_import_and_macro();
  test_control_flow_structure();
  test_function_expression_structure();
  test_new_expression_structure();
  test_top_level_execution_stays_invalid();
  test_data_field_set_and_get_structure();
  test_function_type_structure();
  test_io_target_expression_structure();
  test_character_literal_structure();
  test_lifetime_type_structure();
  return failures == 0 ? 0 : 1;
}
