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

static bool parse_symbols(const char *text, XsSyntaxTree *tree, XsHirSymbolTable *symbols,
                          XsDiagnostics *diagnostics)
{
  XsSource source = {.path = "Members.xs", .text = text, .length = strlen(text)};
  xs_diagnostics_init(diagnostics);
  xs_hir_symbol_table_init(symbols);
  if (!xs_syntax_parse(&source, 101, diagnostics, tree))
    return false;
  return xs_hir_collect_symbols(tree, symbols, diagnostics);
}

static void test_class_member_symbols(void)
{
  const char *text = "module App;\n"
                     "class User {\n"
                     "  name: str;\n"
                     "  fn GetName() => str { return name; }\n"
                     "}\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirMemberSymbolTable members;
  XsDiagnostics diagnostics;
  CHECK(parse_symbols(text, &tree, &symbols, &diagnostics));
  xs_hir_member_symbol_table_init(&members);
  const XsHirSymbol *user = xs_hir_symbol_table_find(&symbols, "App.User");
  CHECK(user != nullptr);
  CHECK(xs_hir_collect_member_symbols(user, nullptr, &members, &diagnostics));
  CHECK(members.count == 2);
  const XsHirMemberSymbol *name = xs_hir_member_symbol_table_find(&members, "App.User", "name");
  const XsHirMemberSymbol *method = xs_hir_member_symbol_table_find(&members, "App.User", "GetName");
  CHECK(name != nullptr && name->kind == XS_HIR_MEMBER_FIELD);
  CHECK(method != nullptr && method->kind == XS_HIR_MEMBER_METHOD);
  CHECK(method != nullptr && strcmp(method->qualified_name, "App.User.GetName") == 0);
  xs_hir_member_symbol_table_free(&members);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_method_merge_uses_last_member_symbol(void)
{
  const char *text = "module App;\n"
                     "class User {\n"
                     "  incomplete fn Merge(value: int);\n"
                     "  incomplete fn Merge(value: str);\n"
                     "}\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirMemberSymbolTable members;
  XsDiagnostics diagnostics;
  CHECK(parse_symbols(text, &tree, &symbols, &diagnostics));
  xs_hir_member_symbol_table_init(&members);
  const XsHirSymbol *user = xs_hir_symbol_table_find(&symbols, "App.User");
  CHECK(xs_hir_collect_member_symbols(user, nullptr, &members, &diagnostics));
  CHECK(members.count == 2);
  const XsHirMemberSymbol *merged = xs_hir_member_symbol_table_find(&members, "App.User", "Merge");
  CHECK(merged != nullptr && merged->kind == XS_HIR_MEMBER_METHOD);
  CHECK(merged != nullptr && merged->span.start_offset > 60);
  xs_hir_member_symbol_table_free(&members);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_duplicate_field_member_errors(void)
{
  const char *text = "module App;\n"
                     "class User {\n"
                     "  value: int;\n"
                     "  value: str;\n"
                     "}\n";
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirMemberSymbolTable members;
  XsDiagnostics diagnostics;
  CHECK(parse_symbols(text, &tree, &symbols, &diagnostics));
  xs_hir_member_symbol_table_init(&members);
  const XsHirSymbol *user = xs_hir_symbol_table_find(&symbols, "App.User");
  CHECK(!xs_hir_collect_member_symbols(user, nullptr, &members, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_member_symbol_table_free(&members);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_macro_generated_field_like_member_symbol(void)
{
  const char *text = "module App;\n"
                     "class User {\n"
                     "  macroRules! make { (): { value: int; }; }\n"
                     "  make!();\n"
                     "}\n";
  XsSource source = {.path = "MacroMembers.xs", .text = text, .length = strlen(text)};
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirMemberSymbolTable members;
  XsMacroDeclarationExpansionSet declarations;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_member_symbol_table_init(&members);
  CHECK(xs_syntax_parse(&source, 102, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(xs_hir_collect_symbols_expanded(&tree, &declarations, &symbols, &diagnostics));
  const XsHirSymbol *user = xs_hir_symbol_table_find(&symbols, "App.User");
  CHECK(xs_hir_collect_member_symbols(user, &declarations, &members, &diagnostics));
  const XsHirMemberSymbol *value = xs_hir_member_symbol_table_find(&members, "App.User", "value");
  CHECK(value != nullptr && value->kind == XS_HIR_MEMBER_FIELD);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_hir_member_symbol_table_free(&members);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_class_member_symbols();
  test_method_merge_uses_last_member_symbol();
  test_duplicate_field_member_errors();
  test_macro_generated_field_like_member_symbol();
  return failures == 0 ? 0 : 1;
}
