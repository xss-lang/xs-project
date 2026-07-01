#include "xs/diagnostic.h"
#include "xs/hir/symbol_table.h"
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

static void test_declaration_macro_symbols(void)
{
  const char *text = "module App;\n"
                     "macroRules! make { (): { incomplete fn Generated(); }; }\n"
                     "make!();\n";
  XsSource source = {.path = "MacroSymbols.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroExpansionReport report;
  XsMacroDeclarationExpansionSet declarations;
  XsHirSymbolTable symbols;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  CHECK(xs_syntax_parse(&source, 81, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_seen == 1);
  CHECK(report.calls_expandable == 1);
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(declarations.count == 1);
  CHECK(xs_hir_collect_symbols_expanded(&tree, &declarations, &symbols, &diagnostics));
  const XsHirSymbol *generated = xs_hir_symbol_table_find(&symbols, "App.Generated");
  CHECK(generated != nullptr);
  CHECK(generated == nullptr || generated->kind == XS_HIR_SYMBOL_FUNCTION);
  CHECK(generated == nullptr || generated->span.file_id == 81);
  xs_hir_symbol_table_free(&symbols);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_declaration_macro_duplicate_symbols(void)
{
  const char *text = "module App;\n"
                     "fn Generated() {}\n"
                     "macroRules! make { (): { incomplete fn Generated(); }; }\n"
                     "make!();\n";
  XsSource source = {.path = "MacroDuplicateSymbols.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsMacroDeclarationExpansionSet declarations;
  XsHirSymbolTable symbols;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  CHECK(xs_syntax_parse(&source, 82, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(!xs_hir_collect_symbols_expanded(&tree, &declarations, &symbols, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_symbol_table_free(&symbols);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_declaration_macro_symbols();
  test_declaration_macro_duplicate_symbols();
  return failures == 0 ? 0 : 1;
}
