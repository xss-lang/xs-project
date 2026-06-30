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

static void test_single_token_fragment_matching(void)
{
  const char *valid = "macroRules! name { ($value:ident): {}; }"
                      "macroRules! lit { ($value:literal): {}; }"
                      "macroRules! life { ($value:lifetime): {}; }"
                      "macroRules! visibility { ($value:vis): {}; }"
                      "fn Main() { name!(value); lit!(42); life!('a); visibility!(public); }";
  XsSource source = {.path = "SingleTokenFragments.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 17, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid_literal = "macroRules! lit { ($value:literal): {}; } fn Main() { lit!(name); }";
  source = (XsSource){.path = "LiteralFragmentInvalid.xs", .text = invalid_literal, .length = strlen(invalid_literal)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 18, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid_lifetime = "macroRules! life { ($value:lifetime): {}; } fn Main() { life!(name); }";
  source =
      (XsSource){.path = "LifetimeFragmentInvalid.xs", .text = invalid_lifetime, .length = strlen(invalid_lifetime)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 19, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_macro_expansion_preparation_report(void)
{
  const char *text = "macroRules! exact { (hello): { world }; }"
                     "macroRules! id { ($value:ident): { $value }; }"
                     "fn Main() { exact!(hello); id!(name); }";
  XsSource source = {.path = "MacroExpansionReady.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 20, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 2);
  CHECK(report.calls_resolved == 2);
  CHECK(report.calls_expandable == 2);
  CHECK(report.calls_deferred == 0);
  CHECK(report.output_tokens_planned == 2);
  CHECK(report.substitutions_planned == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 2);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 1);
  CHECK(expansions.count < 1 || text_is(expansions.items[0].tokens[0].text, "world"));
  CHECK(expansions.count < 2 || expansions.items[1].token_count == 1);
  CHECK(expansions.count < 2 || text_is(expansions.items[1].tokens[0].text, "name"));
  CHECK(expansions.count < 2 || expansions.items[1].tokens[0].from_substitution);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 2);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(statements.count < 2 || statements.items[1].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(statements.count < 2 || xs_syntax_find_first(statements.items[1].statement, XS_SYNTAX_EXPR_IDENTIFIER) != NULL);
  xs_macro_statement_expansion_set_free(&statements);
  if (expansions.count >= 2) {
    XsMacroReparseResult reparse;
    CHECK(xs_macro_reparse_expansion_as_statement(&expansions.items[1], 22, &diagnostics, &reparse));
    const XsSyntaxNode *replacement = xs_macro_reparse_result_statement(&reparse);
    CHECK(replacement != NULL && replacement->kind == XS_SYNTAX_STMT_EXPRESSION);
    CHECK(xs_syntax_find_first(replacement, XS_SYNTAX_EXPR_IDENTIFIER) != NULL);
    xs_macro_reparse_result_free(&reparse);
  }
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *deferred = "macroRules! identity { ($value:expr): { $value }; } fn Main() { identity!(42); }";
  source = (XsSource){.path = "MacroExpansionDeferred.xs", .text = deferred, .length = strlen(deferred)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 21, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_resolved == 1);
  CHECK(report.calls_expandable == 0);
  CHECK(report.calls_deferred == 1);
  CHECK(report.output_tokens_planned == 0);
  CHECK(report.substitutions_planned == 0);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 0);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 0);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_macro_repetition_and_unique_variables();
  test_macro_semantic_validation();
  test_macro_scope_resolution();
  test_single_token_fragment_matching();
  test_macro_expansion_preparation_report();
  return failures == 0 ? 0 : 1;
}
