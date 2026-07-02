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
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
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
                      "  a: int = 1;\n"
                      "  b: bool = true;\n"
                      "  c: str = \"hello\";\n"
                      "  d: char = 'x';\n"
                      "  e: float = 1.0;\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: int = \"bad\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: bool = 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: char = 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: float = true; }\n"));
}

static void test_immutable_local_assignment_errors(void)
{
  const char *valid = "module App;\n"
                      "fn Main() {\n"
                      "  value: int = 1;\n"
                      "  value = 2;\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { val value: int = 1; value = 2; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { const value: int = 1; value = 2; }\n"));
}

static void test_assignment_literal_expression_types(void)
{
  const char *valid = "module App;\n"
                      "fn Main() {\n"
                      "  value: int = 1;\n"
                      "  value = 2;\n"
                      "  flag: bool = false;\n"
                      "  flag = true;\n"
                      "}\n";
  CHECK(check_single_source_expressions(valid));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: int = 1; value = \"bad\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: bool = false; value = 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Main() { value: str = \"ok\"; value = 'x'; }\n"));
}

static void test_return_literal_expression_types(void)
{
  CHECK(check_single_source_expressions("module App;\nfn Count() => int { return 1; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn Flag() => bool { return true; }\n"));
  CHECK(check_single_source_expressions("module App;\nfn Name() => str { return \"xs\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Count() => int { return \"bad\"; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Flag() => bool { return 1; }\n"));
  CHECK(!check_single_source_expressions("module App;\nfn Name() => str { return 'x'; }\n"));
}

static void test_macro_literal_initializer_expression_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { value: int = \"bad\"; }; }\n"
                     "fn Main() { bad!(); }\n";
  CHECK(check_macro_expression_error(main, 96));
}

static void test_macro_immutable_assignment_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { value = 2; }; }\n"
                     "fn Main() { val value: int = 1; bad!(); }\n";
  CHECK(check_macro_expression_error(main, 97));
}

static void test_macro_assignment_literal_expression_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { value = \"bad\"; }; }\n"
                     "fn Main() { value: int = 1; bad!(); }\n";
  CHECK(check_macro_expression_error(main, 98));
}

static void test_macro_return_literal_expression_errors(void)
{
  const char *main = "module App;\n"
                     "macroRules! bad { (): { return \"bad\"; }; }\n"
                     "fn Main() => int { bad!(); }\n";
  CHECK(check_macro_expression_error(main, 99));
}

int main(void)
{
  test_literal_initializer_expression_types();
  test_immutable_local_assignment_errors();
  test_assignment_literal_expression_types();
  test_return_literal_expression_types();
  test_macro_literal_initializer_expression_errors();
  test_macro_immutable_assignment_errors();
  test_macro_assignment_literal_expression_errors();
  test_macro_return_literal_expression_errors();
  return failures == 0 ? 0 : 1;
}
