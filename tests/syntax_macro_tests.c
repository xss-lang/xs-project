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
  CHECK(matcher == NULL || (matcher->flags & XS_SYNTAX_FLAG_REPETITION_COMMA) != 0);
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

static void test_macro_scope_resolution(void)
{
  const char *call_before_definition = "fn Main() { later!(); macroRules! later { (): {}; } }";
  XsSource source = {.path = "MacroScope.xs", .text = call_before_definition, .length = strlen(call_before_definition)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 14, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *out_of_scope = "fn Main() { { macroRules! local { (): {}; } } local!(); }";
  source = (XsSource){.path = "MacroOutOfScope.xs", .text = out_of_scope, .length = strlen(out_of_scope)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 15, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *token_mismatch = "macroRules! exact { (hello): {}; } fn Main() { exact!(world); }";
  source = (XsSource){.path = "MacroMismatch.xs", .text = token_mismatch, .length = strlen(token_mismatch)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 16, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_macro_repetition_and_unique_variables();
  test_macro_semantic_validation();
  test_macro_scope_resolution();
  return failures == 0 ? 0 : 1;
}
