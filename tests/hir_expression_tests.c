/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/diagnostic.h"
#include "xs/hir/expression_check.h"
#include "xs/hir/symbol_table.h"
#include "xs/hir/type_resolution.h"
#include "xs/macro.h"
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

static bool add_file(const char *text, uint64_t file_id, XsSyntaxTree *tree, XsHirSymbolTable *symbols,
                     XsDiagnostics *diagnostics)
{
  XsSource source = {.path = "HirExpressions.xs", .text = text, .length = strlen(text)};
  if (!xs_syntax_parse(&source, file_id, diagnostics, tree))
    return false;
  return xs_hir_collect_symbols(tree, symbols, diagnostics);
}

static bool check_single_source_expressions(const char *text)
{
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  bool success = add_file(text, 72, &tree, &symbols, &diagnostics);
  if (success)
    success = xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics);
  if (success)
    success = xs_hir_resolve_types(&tree, &symbols, &imports, &diagnostics);
  if (success)
    success = xs_hir_check_expression_types(&tree, &diagnostics);
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
  return success;
}

static bool check_macro_expression_error(const char *main, uint64_t file_id)
{
  XsSource source = {.path = "MacroExpressionTypes.xs", .text = main, .length = strlen(main)};
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsMacroStatementExpansionSet statements;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  bool success = xs_syntax_parse(&source, file_id, &diagnostics, &tree);
  if (success)
    success = xs_macro_validate(&tree, &diagnostics);
  if (success)
    success = xs_macro_expand_statements(&tree, &diagnostics, &statements);
  if (success)
    success = xs_hir_collect_symbols(&tree, &symbols, &diagnostics);
  if (success)
    success = xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics);
  if (success)
    success = xs_hir_resolve_types_expanded(&tree, &statements, &symbols, &imports, &diagnostics);
  if (success)
    success = !xs_hir_check_expression_types_with_macros(&tree, nullptr, &statements, &diagnostics) &&
              xs_diagnostics_has_error(&diagnostics);
  xs_macro_statement_expansion_set_free(&statements);
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
  return success;
}

static void test_literal_initializer_expression_types(void)
{
  const char *valid = "module App;\n"
                      "fn Main() {\n"
                      "  a: Int = 1;\n"
                      "  b: Bool = true;\n"
                      "  c: Str = \"hello\";\n"
                      "  d: Char = 'x';\n"
                      "  e: Float = 1.0;\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Int = \"bad\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Bool = 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Char = 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Float = true; }\n"));
}

static void test_control_flow_initializer_expression_types(void)
{
  const char *valid_if = "module App;\n"
                         "fn Main() {\n"
                         "  value: Int = if (true) { 1; } else { 2; };\n"
                         "}\n";
  const char *valid_match = "module App;\n"
                            "fn Main() {\n"
                            "  value: Str = match (1) { 0 -> { \"zero\"; }, else -> { \"many\"; }, };\n"
                            "}\n";
  CHECK(check_single_source_expressions(valid_if));
  CHECK(check_single_source_expressions(valid_match));
  CHECK(!check_single_source_expressions(
      "module App;\nfn Main() { value: Int = if (true) { \"bad\"; } else { 1; }; }\n"));
  CHECK(!check_single_source_expressions(
      "module App;\nfn Main() { value: Bool = match (1) { 0 -> { true; }, else -> { 1; }, }; }\n"));
}

static void test_binding_reassignment_errors(void)
{
  const char *valid = "module App;\n"
                      "fn Main() {\n"
                      "  value: Int = 1;\n"
                      "  value = 2;\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { val value: Int = 1; value = 2; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { const value: Int = 1; value = 2; }\n"));
}

static void test_constant_initializer_errors(void)
{
  CHECK(check_single_source_expressions("module App;\nfn Main() { const value: Int = 1 + 2; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn Main() { static value: Str = \"xs\"; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn RuntimeValue() => Int { return 1; }\nfn Main() { const "
                                        "value: Int = RuntimeValue(); }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { const value: Int; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { static value: Int; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn RuntimeValue() => Int { return 1; }\nfn Main() { static "
                                         "value: Int = RuntimeValue(); }\n"));
}

static void test_assignment_literal_expression_types(void)
{
  const char *valid = "module App;\n"
                      "fn Main() {\n"
                      "  value: Int = 1;\n"
                      "  value = 2;\n"
                      "  flag: Bool = false;\n"
                      "  flag = true;\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Int = 1; value = \"bad\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Bool = false; value = 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: Str = \"ok\"; value = 'x'; }\n"));
}

static void test_control_flow_assignment_expression_types(void)
{
  const char *valid = "module App;\n"
                      "fn Main() {\n"
                      "  value: Int = 0;\n"
                      "  value = if (true) { 1; } else { 2; };\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions(
      "module App;\nfn Main() { value: Int = 0; value = if (true) { 1; } else { \"bad\"; }; }\n"));
}

static void test_return_literal_expression_types(void)
{
  CHECK(check_single_source_expressions("module App;\nfn Count() => Int { return 1; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn Flag() => Bool { return true; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn Name() => Str { return \"xs\"; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn Main() throws Int { return \"not checked yet\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Count() => Int { return \"bad\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Flag() => Bool { return 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Name() => Str { return 'x'; }\n"));
}

static void test_control_flow_return_expression_types(void)
{
  CHECK(check_single_source_expressions("module App;\nfn Count() => Int { return if (true) { 1; } else { 2; }; }\n"));
  CHECK(check_single_source_expressions(
      "module App;\nfn Name() => Str { return match (1) { 0 -> { \"zero\"; }, else -> { \"many\"; }, }; }\n"));
  CHECK(!check_single_source_expressions(
      "module App;\nfn Count() => Int { return if (true) { 1; } else { \"bad\"; }; }\n"));
  CHECK(!check_single_source_expressions(
      "module App;\nfn Flag() => Bool { return match (1) { 0 -> { true; }, else -> { 1; }, }; }\n"));
}

static void test_macro_literal_initializer_expression_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { value: Int = \"bad\"; }; }\n"
                     "fn Main() { bad!(); }\n";
  CHECK(check_macro_expression_error(main, 96));
}

static void test_macro_binding_reassignment_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { value = 2; }; }\n"
                     "fn Main() { val value: Int = 1; bad!(); }\n";
  CHECK(check_macro_expression_error(main, 97));
}

static void test_macro_assignment_literal_expression_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { value = \"bad\"; }; }\n"
                     "fn Main() { value: Int = 1; bad!(); }\n";
  CHECK(check_macro_expression_error(main, 98));
}

static void test_macro_return_literal_expression_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { return \"bad\"; }; }\n"
                     "fn Main() => Int { bad!(); }\n";
  CHECK(check_macro_expression_error(main, 99));
}

int main(void)
{
  test_literal_initializer_expression_types();
  test_control_flow_initializer_expression_types();
  test_binding_reassignment_errors();
  test_constant_initializer_errors();
  test_assignment_literal_expression_types();
  test_control_flow_assignment_expression_types();
  test_return_literal_expression_types();
  test_control_flow_return_expression_types();
  test_macro_literal_initializer_expression_errors();
  test_macro_binding_reassignment_errors();
  test_macro_assignment_literal_expression_errors();
  test_macro_return_literal_expression_errors();
  return failures == 0 ? 0 : 1;
}
