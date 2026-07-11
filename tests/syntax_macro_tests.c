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
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
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
  CHECK(matcher != nullptr);
  CHECK(matcher == nullptr || (matcher->flags & XS_SYNTAX_FLAG_REPETITION_COMMA) != 0);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_MACRO_EXPANSION_REPETITION) != nullptr);
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
  CHECK(statements.count < 2 ||
        xs_syntax_find_first(statements.items[1].statement, XS_SYNTAX_EXPR_IDENTIFIER) != nullptr);
  const XsSyntaxNode *macro_statement = xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_MACRO_CALL);
  const XsSyntaxNode *found = xs_macro_statement_expansion_find(&statements, macro_statement);
  CHECK(found != nullptr && found->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(xs_macro_statement_expansion_find(&statements, tree.root) == nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  if (expansions.count >= 2)
  {
    XsMacroReparseResult reparse;
    CHECK(xs_macro_reparse_expansion_as_statement(&expansions.items[1], 22, &diagnostics, &reparse));
    const XsSyntaxNode *replacement = xs_macro_reparse_result_statement(&reparse);
    CHECK(replacement != nullptr && replacement->kind == XS_SYNTAX_STMT_EXPRESSION);
    CHECK(xs_syntax_find_first(replacement, XS_SYNTAX_EXPR_IDENTIFIER) != nullptr);
    xs_macro_reparse_result_free(&reparse);
  }
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *nested = "macroRules! id { ($value:ident): { $value }; }"
                       "fn Main() { print(id!(name)); id!(name); }";
  source = (XsSource){.path = "MacroNestedExpression.xs", .text = nested, .length = strlen(nested)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 23, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 2);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *expression = "macroRules! identity { ($value:expr): { $value }; } fn Main() { identity!(42); }";
  source = (XsSource){.path = "MacroExpressionFragment.xs", .text = expression, .length = strlen(expression)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 21, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_resolved == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(report.calls_deferred == 0);
  CHECK(report.output_tokens_planned == 1);
  CHECK(report.substitutions_planned == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 1);
  CHECK(expansions.count < 1 || text_is(expansions.items[0].tokens[0].text, "42"));
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(statements.count < 1 || xs_syntax_find_first(statements.items[0].statement, XS_SYNTAX_EXPR_LITERAL) != nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_statement_fragment_expansion(void)
{
  const char *text = "macroRules! pass { ($body:stmt): { $body }; }"
                     "fn Main() { pass!(value: Missing = None;); }";
  XsSource source = {.path = "MacroStatementFragment.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 24, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_resolved == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(report.calls_deferred == 0);
  CHECK(report.substitutions_planned == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 6);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_VARIABLE);
  CHECK(statements.count < 1 || xs_syntax_find_first(statements.items[0].statement, XS_SYNTAX_TYPE_NAMED) != nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid = "macroRules! pass { ($body:stmt): { $body }; }"
                        "fn Main() { pass!(); }";
  source = (XsSource){.path = "MacroStatementFragmentInvalid.xs", .text = invalid, .length = strlen(invalid)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 26, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_block_fragment_expansion(void)
{
  const char *text = "macroRules! pass { ($body:block): { $body }; }"
                     "fn Main() { pass!({ value: Missing = None; }); }";
  XsSource source = {.path = "MacroBlockFragment.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 25, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 8);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_BLOCK);
  CHECK(statements.count < 1 ||
        xs_syntax_find_first(statements.items[0].statement, XS_SYNTAX_STMT_VARIABLE) != nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid = "macroRules! pass { ($body:block): { $body }; }"
                        "fn Main() { pass!(); }";
  source = (XsSource){.path = "MacroBlockFragmentInvalid.xs", .text = invalid, .length = strlen(invalid)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 27, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_type_fragment_expansion(void)
{
  const char *text = "macroRules! declare { ($kind:ty): { value: $kind = None }; }"
                     "fn Main() { declare!(Missing); }";
  XsSource source = {.path = "MacroTypeFragment.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 28, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 5);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_VARIABLE);
  CHECK(statements.count < 1 || xs_syntax_find_first(statements.items[0].statement, XS_SYNTAX_TYPE_NAMED) != nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid = "macroRules! declare { ($kind:ty): { value: $kind = None }; }"
                        "fn Main() { declare!(); }";
  source = (XsSource){.path = "MacroTypeFragmentInvalid.xs", .text = invalid, .length = strlen(invalid)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 29, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_path_fragment_expansion(void)
{
  const char *text = "macroRules! call { ($target:path): { $target(); }; }"
                     "fn Main() { call!(Missing.Call); }";
  XsSource source = {.path = "MacroPathFragment.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 30, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 6);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_EXPRESSION);
  CHECK(statements.count < 1 ||
        xs_syntax_find_first(statements.items[0].statement, XS_SYNTAX_EXPR_IDENTIFIER) != nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid = "macroRules! call { ($target:path): { $target(); }; }"
                        "fn Main() { call!(); }";
  source = (XsSource){.path = "MacroPathFragmentInvalid.xs", .text = invalid, .length = strlen(invalid)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 31, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_pattern_fragment_expansion(void)
{
  const char *text =
      "macroRules! matchIt { ($case:pat): { match (value) { $case -> { return; }, else -> { return; }, } }; }"
      "fn Main() { matchIt!(None); }";
  XsSource source = {.path = "MacroPatternFragment.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 34, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(report.substitutions_planned == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(statements.count < 1 || statements.items[0].statement->kind == XS_SYNTAX_STMT_MATCH);
  CHECK(statements.count < 1 ||
        xs_syntax_find_first(statements.items[0].statement, XS_SYNTAX_PATTERN_LITERAL) != nullptr);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_declaration_macro_expansion(void)
{
  const char *text = "macroRules! make { (): { incomplete fn Generated(); }; } make!();";
  XsSource source = {.path = "MacroDeclarationExpansion.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  XsMacroDeclarationExpansionSet declarations;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 32, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 1);
  CHECK(expansions.count < 1 || expansions.items[0].token_count == 6);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 0);
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(declarations.count == 1);
  CHECK(declarations.count < 1 || declarations.items[0].declaration_count == 1);
  CHECK(declarations.count < 1 ||
        xs_syntax_find_first(declarations.items[0].reparse.tree.root, XS_SYNTAX_DECL_FUNCTION) != nullptr);
  const XsSyntaxNode *macro_declaration = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_MACRO_CALL);
  const XsMacroDeclarationExpansion *found = xs_macro_declaration_expansion_find(&declarations, macro_declaration);
  CHECK(found != nullptr && found->declaration_count == 1);
  CHECK(xs_macro_declaration_expansion_find(&declarations, tree.root) == nullptr);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid = "macroRules! make { (): {}; } make!();";
  source = (XsSource){.path = "MacroDeclarationExpansionInvalid.xs", .text = invalid, .length = strlen(invalid)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 33, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(!xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_imported_panic_macros_resolve_without_expansion(void)
{
  const char *text = "imports Panic;\nfn Main() { assert!(true); assert_eq!(1, 1); assert_ne!(1, 2); panic!(); }\n";
  XsSource source = {.path = "PanicMacros.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionSet expansions;
  XsMacroStatementExpansionSet statements;
  XsMacroExpandedStatementSet expanded;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 34, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_tokens(&tree, &diagnostics, &expansions));
  CHECK(expansions.count == 0);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 0);
  const XsSyntaxNode *block = xs_syntax_find_first(tree.root, XS_SYNTAX_STMT_BLOCK);
  CHECK(xs_macro_expand_child_statements(block, &statements, &diagnostics, &expanded));
  CHECK(expanded.count == 4);
  CHECK(expanded.count < 1 || expanded.items[0].statement->kind == XS_SYNTAX_STMT_MACRO_CALL);
  xs_macro_expanded_statement_set_free(&expanded);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_expansion_set_free(&expansions);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *selected = "from Panic imports debug_assert, debug_assert_eq;\n"
                         "fn Main() { debug_assert!(true); debug_assert_eq!(1, 1); }\n";
  source = (XsSource){.path = "SelectedPanicMacros.xs", .text = selected, .length = strlen(selected)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 35, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *missing_import = "fn Main() { panic!(); }\n";
  source = (XsSource){.path = "MissingPanicImport.xs", .text = missing_import, .length = strlen(missing_import)};
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 36, &diagnostics, &tree));
  CHECK(!xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
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
  test_statement_fragment_expansion();
  test_block_fragment_expansion();
  test_type_fragment_expansion();
  test_path_fragment_expansion();
  test_pattern_fragment_expansion();
  test_declaration_macro_expansion();
  test_imported_panic_macros_resolve_without_expansion();
  return failures == 0 ? 0 : 1;
}
