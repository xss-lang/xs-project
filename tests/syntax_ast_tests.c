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

static void test_function_tree(void)
{
  const char *text = "fn Add(a: int, b: int) => int {\n    result: int = a + b;\n    return result;\n}\n";
  XsSource source = {.path = "Add.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 42, &diagnostics, &tree));
  CHECK(tree.root != NULL && tree.root->kind == XS_SYNTAX_FILE);
  CHECK(tree.root->child_count == 1);
  const XsSyntaxNode *function = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_FUNCTION);
  CHECK(function != NULL);
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

static void test_macro_repetition_and_unique_variables(void)
{
  const char *valid = "macroRules! many { ($( $value:expr ),*): { $( $value )* }; }";
  XsSource source = {.path = "Macros.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 10, &diagnostics, &tree));
  const XsSyntaxNode *matcher = xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_MATCHER_REPETITION);
  CHECK(matcher != NULL);
  CHECK((matcher->flags & XS_SYNTAX_FLAG_REPETITION_COMMA) != 0);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_EXPANSION_REPETITION) != NULL);
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid = "macroRules! duplicate { ($value:expr): {}; ($value:ident): {}; }";
  source = (XsSource){.path = "InvalidMacros.xs", .text = invalid, .length = strlen(invalid)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 11, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_macro_semantic_validation(void)
{
  const char *depth_error = "macroRules! bad { ($( $value:expr ),*): { $value }; }";
  XsSource source = {.path = "Depth.xs", .text = depth_error, .length = strlen(depth_error)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 12, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *recursion = "macroRules! first { (): { second!(); }; }"
                          "macroRules! second { (): { first!(); }; }";
  source = (XsSource){.path = "Recursion.xs", .text = recursion, .length = strlen(recursion)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 13, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
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

int main(void)
{
  test_function_tree();
  test_module_import_and_macro();
  test_control_flow_structure();
  test_macro_repetition_and_unique_variables();
  test_macro_semantic_validation();
  return failures == 0 ? 0 : 1;
}
