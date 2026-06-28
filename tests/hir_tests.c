#include "xs/diagnostic.h"
#include "xs/hir/symbol_table.h"
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

static bool parse_and_collect(const char *text, XsSyntaxTree *tree, XsHirSymbolTable *symbols,
                              XsDiagnostics *diagnostics)
{
  XsSource source = {.path = "Symbols.xs", .text = text, .length = strlen(text)};
  xs_diagnostics_init(diagnostics);
  xs_hir_symbol_table_init(symbols);
  if (!xs_syntax_parse(&source, 21, diagnostics, tree))
    return false;
  return xs_hir_collect_symbols(tree, symbols, diagnostics);
}

static void free_all(XsSyntaxTree *tree, XsHirSymbolTable *symbols, XsDiagnostics *diagnostics)
{
  xs_hir_symbol_table_free(symbols);
  xs_syntax_tree_free(tree);
  xs_diagnostics_free(diagnostics);
}

static void test_module_namespace_symbols(void)
{
  const char *text = "module App;\n"
                     "namespace Core;\n"
                     "public fn Main() {}\n"
                     "internal class Service {}\n"
                     "namespace Util;\n"
                     "data Result { value: int; }\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsDiagnostics diagnostics;
  CHECK(parse_and_collect(text, &tree, &symbols, &diagnostics));
  CHECK(symbols.count == 3);
  const XsHirSymbol *main = xs_hir_symbol_table_find(&symbols, "App.Core.Main");
  CHECK(main != NULL);
  CHECK(main != NULL && main->kind == XS_HIR_SYMBOL_FUNCTION);
  CHECK(main != NULL && main->visibility == XS_SYNTAX_VISIBILITY_PUBLIC);
  CHECK(xs_hir_symbol_table_find(&symbols, "App.Core.Service") != NULL);
  CHECK(xs_hir_symbol_table_find(&symbols, "App.Util.Result") != NULL);
  free_all(&tree, &symbols, &diagnostics);
}

static void test_duplicate_names_in_namespace(void)
{
  const char *text = "module App;\n"
                     "namespace Core;\n"
                     "fn Twice() {}\n"
                     "class Twice {}\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsDiagnostics diagnostics;
  CHECK(!parse_and_collect(text, &tree, &symbols, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  free_all(&tree, &symbols, &diagnostics);
}

static void test_same_name_in_different_namespace(void)
{
  const char *text = "module App;\n"
                     "namespace First;\n"
                     "fn Value() {}\n"
                     "namespace Second;\n"
                     "fn Value() {}\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsDiagnostics diagnostics;
  CHECK(parse_and_collect(text, &tree, &symbols, &diagnostics));
  CHECK(symbols.count == 2);
  CHECK(xs_hir_symbol_table_find(&symbols, "App.First.Value") != NULL);
  CHECK(xs_hir_symbol_table_find(&symbols, "App.Second.Value") != NULL);
  free_all(&tree, &symbols, &diagnostics);
}

static bool add_file_symbols(const char *text, uint64_t file_id, XsSyntaxTree *tree, XsHirSymbolTable *symbols,
                             XsDiagnostics *diagnostics)
{
  XsSource source = {.path = "File.xs", .text = text, .length = strlen(text)};
  if (!xs_syntax_parse(&source, file_id, diagnostics, tree))
    return false;
  return xs_hir_collect_symbols(tree, symbols, diagnostics);
}

static void test_import_resolution(void)
{
  const char *library = "module Math;\n"
                        "public fn Add() {}\n"
                        "private fn Hidden() {}\n";
  const char *main = "module App;\n"
                     "imports Math;\n"
                     "froms Math imports Add as Sum;\n"
                     "fn Main() {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(library, 31, &library_tree, &symbols, &diagnostics));
  CHECK(add_file_symbols(main, 32, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_import_scope_find(&imports, "Math.Add") != NULL);
  CHECK(xs_hir_import_scope_find(&imports, "Math.Hidden") == NULL);
  const XsHirImportBinding *sum = xs_hir_import_scope_find(&imports, "Sum");
  CHECK(sum != NULL);
  CHECK(sum != NULL && strcmp(sum->symbol->qualified_name, "Math.Add") == 0);
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_public_namespace_exports_default_symbols(void)
{
  const char *library = "module Math;\n"
                        "public namespace Advanced;\n"
                        "fn Add() {}\n"
                        "private fn Hidden() {}\n";
  const char *main = "module App;\n"
                     "imports Math.Advanced;\n"
                     "froms Math.Advanced imports Add;\n"
                     "fn Main() { Math.Advanced.Add(); Add(); }\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(library, 35, &library_tree, &symbols, &diagnostics));
  const XsHirSymbol *add = xs_hir_symbol_table_find(&symbols, "Math.Advanced.Add");
  const XsHirSymbol *hidden = xs_hir_symbol_table_find(&symbols, "Math.Advanced.Hidden");
  CHECK(add != NULL && add->visibility == XS_SYNTAX_VISIBILITY_PUBLIC);
  CHECK(hidden != NULL && hidden->visibility == XS_SYNTAX_VISIBILITY_PRIVATE);
  CHECK(add_file_symbols(main, 36, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_import_scope_find(&imports, "Math.Advanced.Add") != NULL);
  CHECK(xs_hir_import_scope_find(&imports, "Math.Advanced.Hidden") == NULL);
  CHECK(xs_hir_validate_name_uses(&main_tree, &symbols, &imports, &diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_import_errors(void)
{
  const char *library = "module Math;\n"
                        "private fn Hidden() {}\n";
  const char *main = "module App;\n"
                     "froms Math imports Hidden;\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(library, 41, &library_tree, &symbols, &diagnostics));
  CHECK(add_file_symbols(main, 42, &main_tree, &symbols, &diagnostics));
  CHECK(!xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_name_use_resolution(void)
{
  const char *library = "module Math.Advanced;\n"
                        "public fn Add(a: int, b: int) => int { return a + b; }\n";
  const char *main = "module App;\n"
                     "imports Math.Advanced;\n"
                     "froms Math.Advanced imports Add as Sum;\n"
                     "fn Main() {\n"
                     "  first: int = Math.Advanced.Add(1, 2);\n"
                     "  second: int = Sum(3, 4);\n"
                     "}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(library, 51, &library_tree, &symbols, &diagnostics));
  CHECK(add_file_symbols(main, 52, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_validate_name_uses(&main_tree, &symbols, &imports, &diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_name_use_errors(void)
{
  const char *main = "module App;\n"
                     "fn Main() { Missing.Call(); }\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(main, 61, &tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_validate_name_uses(&tree, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_private_qualified_name_visibility(void)
{
  const char *library = "module Math;\n"
                        "private fn Hidden() {}\n";
  const char *main = "module App;\n"
                     "fn Main() { Math.Hidden(); }\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(library, 71, &library_tree, &symbols, &diagnostics));
  CHECK(add_file_symbols(main, 72, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_validate_name_uses(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_private_same_namespace_name_visibility(void)
{
  const char *text = "module Math;\n"
                     "private fn Hidden() {}\n"
                     "fn Main() { Math.Hidden(); }\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file_symbols(text, 73, &tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_validate_name_uses(&tree, &symbols, &imports, &diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_module_namespace_symbols();
  test_duplicate_names_in_namespace();
  test_same_name_in_different_namespace();
  test_import_resolution();
  test_public_namespace_exports_default_symbols();
  test_import_errors();
  test_name_use_resolution();
  test_name_use_errors();
  test_private_qualified_name_visibility();
  test_private_same_namespace_name_visibility();
  return failures == 0 ? 0 : 1;
}
