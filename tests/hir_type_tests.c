/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/diagnostic.h"
#include "xs/hir/symbol_table.h"
#include "xs/hir/type_info.h"
#include "xs/hir/type_resolution.h"
#include "xs/macro.h"
#include "xs/syntax_parser.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while(0)

static bool add_file(const char *text, uint64_t file_id, XsSyntaxTree *tree, XsHirSymbolTable *symbols,
                     XsDiagnostics *diagnostics)
{
  XsSource source = {.path = "HirTypes.xs", .text = text, .length = strlen(text)};
  if(!xs_syntax_parse(&source, file_id, diagnostics, tree))
    return false;
  return xs_hir_collect_symbols(tree, symbols, diagnostics);
}

static bool check_single_source(const char *text)
{
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  bool success = add_file(text, 71, &tree, &symbols, &diagnostics);
  if(success)
    success = xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics);
  if(success)
    success = xs_hir_resolve_types(&tree, &symbols, &imports, &diagnostics);
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
  return success;
}

static void test_hir_primitive_info(void)
{
  const XsHirPrimitiveInfo *string = xs_hir_primitive_find("Str", 3);
  CHECK(string != nullptr && string->bit_width == 0 && string->code_unit_bit_width == 16 && string->is_text &&
        !string->has_xlil_type);
  const XsHirPrimitiveInfo *boolean = xs_hir_primitive_find("Bool", 4);
  CHECK(boolean != nullptr && boolean->bit_width == 1 && boolean->has_xlil_type &&
        boolean->xlil_type.kind == XS_LIL_TYPE_BOOL);
  const XsHirPrimitiveInfo *byte = xs_hir_primitive_find("Byte", 4);
  CHECK(byte != nullptr && byte->bit_width == 8 && !byte->is_signed && byte->has_xlil_type &&
        byte->xlil_type.kind == XS_LIL_TYPE_U8);
  const XsHirPrimitiveInfo *sbyte = xs_hir_primitive_find("SByte", 5);
  CHECK(sbyte != nullptr && sbyte->bit_width == 8 && sbyte->is_signed && sbyte->has_xlil_type &&
        sbyte->xlil_type.kind == XS_LIL_TYPE_I8);
  const XsHirPrimitiveInfo *character = xs_hir_primitive_find("Char", 4);
  CHECK(character != nullptr && character->bit_width == 16 && character->has_xlil_type &&
        character->xlil_type.kind == XS_LIL_TYPE_U16);
}

static void test_primitive_and_generic_types(void)
{
  const char *text = "module App;\n"
                     "fn Keep<T>(a: Str, b: Bool, c: Byte, d: SByte, e: Char, f: Short, g: Long, h: Int,\n"
                     "           i: Integer, j: UShort, k: ULong, l: UInt, m: UInteger,\n"
                     "           n: SFloat, o: Float, p: T) -> T { return p; }\n";
  CHECK(check_single_source(text));
  CHECK(!check_single_source("module App;\nfn Main(value: Float16) {}\n"));
  CHECK(!check_single_source("module App;\nfn Main(value: double) {}\n"));
}

static void test_local_user_type(void)
{
  const char *text = "module App;\n"
                     "data User { name: Str; }\n"
                     "fn Main(value: User) {}\n";
  CHECK(check_single_source(text));
}

static void test_generic_type_arity(void)
{
  const char *valid = "module App;\n"
                      "class Box<T> { value: T; }\n"
                      "data Pair<T, U> { first: T; second: U; }\n"
                      "fn Main(box: Box<Int>, pair: Pair<Str, Int>) {}\n";
  CHECK(check_single_source(valid));
  const char *missing = "module App;\n"
                        "class Box<T> { value: T; }\n"
                        "fn Main(box: Box) {}\n";
  CHECK(!check_single_source(missing));
  const char *too_few = "module App;\n"
                        "data Pair<T, U> { first: T; second: U; }\n"
                        "fn Main(pair: Pair<Int>) {}\n";
  CHECK(!check_single_source(too_few));
  const char *too_many = "module App;\n"
                         "class Box<T> { value: T; }\n"
                         "fn Main(box: Box<Int, Str>) {}\n";
  CHECK(!check_single_source(too_many));
}

static void test_standard_generic_types(void)
{
  const char *valid = "module App;\n"
                      "fn Read() -> Optional<Str> { return None; }\n"
                      "fn ReadCanonical() -> std::optional::Optional<Str> { return None; }\n"
                      "fn Save() -> Result<Int> { return Ok(1); }\n"
                      "fn Load() -> Result<Int, Error> { return Ok(1); }\n"
                      "fn LoadCanonical() -> std::result::Result<Int, Error> { return Ok(1); }\n"
                      "fn Compact() -> Result<Int, Error> { return Ok(1); }\n";
  CHECK(check_single_source(valid));
  CHECK(!check_single_source("module App;\nfn Missing() -> Optional { return None; }\n"));
  CHECK(!check_single_source("module App;\nfn Missing() -> Result { return Ok(1); }\n"));
  CHECK(!check_single_source("module App;\nfn TooMany() -> Result<Int, Error, Int> { return "
                             "Ok(1); }\n"));
  CHECK(!check_single_source("module App;\nfn BadError() -> Error<Int> { return Ok(1); }\n"));
}

static void test_else_type_placeholder(void)
{
  const char *valid = "module App;\n"
                      "class Box<T> { value: T; }\n"
                      "fn Main(box: Box<else>) {}\n";
  CHECK(check_single_source(valid));
  CHECK(!check_single_source("module App;\nclass Box<T> { value: T; }\nfn Main(box: Box<_>) {}\n"));
}

static void test_standard_cffi_types(void)
{
  const char *valid = "module App;\n"
                      "imports cffi;\n"
                      "data NativeLibrary { handle: std::cffi::Handle<NativeLibrary>; }\n"
                      "#[repr(C)]\n"
                      "extern \"C\" {\n"
                      "  fn puts(text: std::cffi::CStr) -> Int;\n"
                      "  fn free(ptr: std::cffi::RawPtr<Byte>);\n"
                      "  fn read(out: std::cffi::Out<Int>) -> Int;\n"
                      "  static stdin_handle: std::cffi::RawPtr<std::cffi::File>;\n"
                      "}\n"
                      "fn Load(symbol: std::cffi::Symbol<fn() -> Int>, library: std::cffi::DynamicLibrary) {}\n";
  CHECK(check_single_source(valid));
  CHECK(!check_single_source("module App;\nimports cffi;\nextern \"C\" { fn Bad(ptr: std::cffi::RawPtr); }\n"));
  CHECK(!check_single_source("module App;\nimports cffi;\nextern \"C\" { fn Bad(ptr: std::cffi::CStr<Int>); }\n"));
  CHECK(!check_single_source("module App;\nfn Bad(ptr: std::cffi::CStr) {}\n"));
}

static void test_duplicate_generic_parameter_names(void)
{
  CHECK(!check_single_source("module App;\nclass Box<T, T> { value: T; }\n"));
  CHECK(!check_single_source("module App;\ninterface Reader<T, T> { fn Read(value: T); }\n"));
  CHECK(!check_single_source("module App;\nfn Keep<T, T>(value: T) -> T { return value; }\n"));
}

static void test_imported_user_type(void)
{
  const char *library = "module Model;\n"
                        "public data User { name: Str; }\n";
  const char *main = "module App;\n"
                     "using Model::User;\n"
                     "fn Main(value: User) {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file(library, 81, &library_tree, &symbols, &diagnostics));
  CHECK(add_file(main, 82, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_resolve_types(&main_tree, &symbols, &imports, &diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_public_namespace_exports_default_type(void)
{
  const char *library = "module Model;\n"
                        "public namespace Records;\n"
                        "data User { name: Str; }\n";
  const char *main = "module App;\n"
                     "imports Model::Records;\n"
                     "fn Main(value: Model::Records::User) {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file(library, 85, &library_tree, &symbols, &diagnostics));
  const XsHirSymbol *user = xs_hir_symbol_table_find(&symbols, "Model.Records.User");
  CHECK(user != nullptr && user->visibility == XS_SYNTAX_VISIBILITY_PUBLIC);
  CHECK(add_file(main, 86, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_resolve_types(&main_tree, &symbols, &imports, &diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_public_qualified_type_requires_import(void)
{
  const char *library = "module Model;\n"
                        "public data User { name: Str; }\n";
  const char *main = "module App;\n"
                     "fn Main(value: Model::User) {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file(library, 89, &library_tree, &symbols, &diagnostics));
  CHECK(add_file(main, 90, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_resolve_types(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_private_qualified_type_visibility(void)
{
  const char *library = "module Model;\n"
                        "private data Secret { value: Int; }\n";
  const char *main = "module App;\n"
                     "fn Main(value: Model::Secret) {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file(library, 83, &library_tree, &symbols, &diagnostics));
  CHECK(add_file(main, 84, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_resolve_types(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_private_same_namespace_type_visibility(void)
{
  const char *text = "module Model;\n"
                     "private data Secret { value: Int; }\n"
                     "fn Main(value: Model::Secret) {}\n";
  CHECK(check_single_source(text));
}

static void test_private_same_namespace_different_file_type_visibility(void)
{
  const char *library = "module Model;\n"
                        "private data Secret { value: Int; }\n";
  const char *main = "module Model;\n"
                     "fn Main(value: Model::Secret) {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file(library, 87, &library_tree, &symbols, &diagnostics));
  CHECK(add_file(main, 88, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_resolve_types(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_generic_constraints(void)
{
  const char *valid = "module App;\n"
                      "interface Printable { fn Print(); }\n"
                      "fn PrintValue<T: Printable>(value: T) {}\n";
  CHECK(check_single_source(valid));
  const char *multiple = "module App;\n"
                         "interface Runnable { fn Run(); }\n"
                         "interface Printable { fn Print(); }\n"
                         "fn Execute<T: Runnable, Printable, U: Runnable>(value: T, worker: U) {}\n";
  CHECK(check_single_source(multiple));
  const char *generic = "module App;\n"
                        "interface Parser<T> { fn Parse(value: T); }\n"
                        "fn UseParser<T: Parser<Int>>(value: T) {}\n";
  CHECK(check_single_source(generic));
  const char *invalid = "module App;\n"
                        "data NotInterface { value: Int; }\n"
                        "fn PrintValue<T: NotInterface>(value: T) {}\n";
  CHECK(!check_single_source(invalid));
  const char *invalid_generic = "module App;\n"
                                "class Box<T> { value: T; }\n"
                                "fn UseBox<T: Box<Int>>(value: T) {}\n";
  CHECK(!check_single_source(invalid_generic));
}

static void test_imported_generic_constraint(void)
{
  const char *library = "module Contract;\n"
                        "public interface Runnable { fn Run(); }\n";
  const char *main = "module App;\n"
                     "using Contract::Runnable;\n"
                     "fn Execute<T: Runnable>(value: T) {}\n";
  XsSyntaxTree library_tree;
  XsSyntaxTree main_tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(add_file(library, 91, &library_tree, &symbols, &diagnostics));
  CHECK(add_file(main, 92, &main_tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&main_tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_resolve_types(&main_tree, &symbols, &imports, &diagnostics));
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&main_tree);
  xs_syntax_tree_free(&library_tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_unknown_type_errors(void)
{
  CHECK(!check_single_source("module App;\nfn Main(value: int64) {}\n"));
  CHECK(!check_single_source("module App;\nfn Main(value: Missing) {}\n"));
}

static void test_expanded_macro_type_errors(void)
{
  const char *main = "module App;\n"
                     "macro_rules! bad { () -> { value: Missing = None }; }\n"
                     "fn Main() { bad!(); }\n";
  XsSource source = {.path = "MacroTypes.xs", .text = main, .length = strlen(main)};
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsMacroExpansionReport report;
  XsMacroStatementExpansionSet statements;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(xs_syntax_parse(&source, 93, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(xs_hir_collect_symbols(&tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_resolve_types(&tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_resolve_types_expanded(&tree, &statements, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_macro_statement_expansion_set_free(&statements);
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_type_fragment_macro_type_errors(void)
{
  const char *main = "module App;\n"
                     "macro_rules! declare { ($kind:ty) -> { value: $kind = None }; }\n"
                     "fn Main() { declare!(Missing); }\n";
  XsSource source = {.path = "MacroTypeFragmentTypes.xs", .text = main, .length = strlen(main)};
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  XsHirImportScope imports;
  XsMacroExpansionReport report;
  XsMacroStatementExpansionSet statements;
  XsDiagnostics diagnostics;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  xs_hir_import_scope_init(&imports);
  CHECK(xs_syntax_parse(&source, 94, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_prepare_expansion(&tree, &diagnostics, &report));
  CHECK(report.calls_expandable == 1);
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(statements.count == 1);
  CHECK(xs_hir_collect_symbols(&tree, &symbols, &diagnostics));
  CHECK(xs_hir_resolve_imports(&tree, &symbols, &imports, &diagnostics));
  CHECK(xs_hir_resolve_types(&tree, &symbols, &imports, &diagnostics));
  CHECK(!xs_hir_resolve_types_expanded(&tree, &statements, &symbols, &imports, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_macro_statement_expansion_set_free(&statements);
  xs_hir_import_scope_free(&imports);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_hir_primitive_info();
  test_primitive_and_generic_types();
  test_local_user_type();
  test_generic_type_arity();
  test_standard_generic_types();
  test_else_type_placeholder();
  test_standard_cffi_types();
  test_duplicate_generic_parameter_names();
  test_imported_user_type();
  test_public_namespace_exports_default_type();
  test_public_qualified_type_requires_import();
  test_private_qualified_type_visibility();
  test_private_same_namespace_type_visibility();
  test_private_same_namespace_different_file_type_visibility();
  test_generic_constraints();
  test_imported_generic_constraint();
  test_unknown_type_errors();
  test_expanded_macro_type_errors();
  test_type_fragment_macro_type_errors();
  return failures == 0 ? 0 : 1;
}
