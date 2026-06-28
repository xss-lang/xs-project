#include "xs/hir/symbol_table.h"
#include "xs/lil.h"
#include "xs/mir.h"
#include "xs/mir/hir_lowering.h"
#include "xs/mir/xlil_lowering.h"
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

static void test_module_and_text_writer(void)
{
  XsMirError error = {0};
  XsMirModule *module = NULL;
  CHECK(xs_mir_module_create("project", &module, &error) == XS_MIR_OK);
  CHECK(module != NULL);
  CHECK(strcmp(xs_mir_module_name(module), "project") == 0);
  const XsMirType parameters[] = {{.kind = XS_LIL_TYPE_I64}, {.kind = XS_LIL_TYPE_I64}};
  CHECK(xs_mir_module_add_function_declaration(module, "App.Add", (XsMirType){.kind = XS_LIL_TYPE_I64}, parameters, 2,
                                               &error) == XS_MIR_OK);
  CHECK(xs_mir_module_function_count(module) == 1);

  FILE *stream = tmpfile();
  if (stream == NULL) {
    ++failures;
    xs_mir_module_destroy(module);
    return;
  }
  CHECK(xs_mir_module_write_text(module, stream, &error) == XS_MIR_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[128] = {0};
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, "mir module project\n") != NULL);
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, "declare fn App.Add(i64, i64) -> i64\n") != NULL);
  fclose(stream);
  xs_mir_module_destroy(module);
}

static void test_hir_function_declaration_lowering(void)
{
  const char *text = "module App;\n"
                     "fn Add(a: int, b: int) => int { return a + b; }\n"
                     "data User { value: int; }\n";
  XsSource source = {.path = "MirLowering.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsHirSymbolTable symbols;
  xs_diagnostics_init(&diagnostics);
  xs_hir_symbol_table_init(&symbols);
  CHECK(xs_syntax_parse(&source, 101, &diagnostics, &tree));
  CHECK(xs_hir_collect_symbols(&tree, &symbols, &diagnostics));

  XsMirError error = {0};
  XsMirModule *module = NULL;
  CHECK(xs_mir_module_create("project", &module, &error) == XS_MIR_OK);
  CHECK(xs_mir_module_add_hir_function_declarations(module, &symbols, &error) == XS_MIR_OK);
  CHECK(xs_mir_module_function_count(module) == 1);
  const XsMirFunction *function = xs_mir_module_function_at(module, 0);
  CHECK(function != NULL && strcmp(xs_mir_function_name(function), "App.Add") == 0);
  CHECK(xs_mir_function_return_type(function).kind == XS_LIL_TYPE_I64);
  CHECK(xs_mir_function_parameter_count(function) == 2);
  xs_mir_module_destroy(module);
  xs_hir_symbol_table_free(&symbols);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_xlil_declaration_lowering(void)
{
  XsMirError error = {0};
  XsMirModule *mir = NULL;
  CHECK(xs_mir_module_create("project", &mir, &error) == XS_MIR_OK);
  const XsMirType parameters[] = {{.kind = XS_LIL_TYPE_BOOL}};
  CHECK(xs_mir_module_add_function_declaration(mir, "App.Check", (XsMirType){.kind = XS_LIL_TYPE_BOOL}, parameters, 1,
                                               &error) == XS_MIR_OK);
  XsLilError lil_error = {0};
  XsLilModule *xlil = NULL;
  CHECK(xs_lil_module_create("project", &xlil, &lil_error) == XS_LIL_OK);
  CHECK(xs_lil_module_add_mir_function_declarations(xlil, mir, &error) == XS_MIR_OK);
  CHECK(xs_lil_module_function_count(xlil) == 1);
  xs_lil_module_destroy(xlil);
  xs_mir_module_destroy(mir);
}

int main(void)
{
  test_module_and_text_writer();
  test_hir_function_declaration_lowering();
  test_xlil_declaration_lowering();
  return failures == 0 ? 0 : 1;
}
