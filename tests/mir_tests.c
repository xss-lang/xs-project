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

static void test_function_definition_blocks_and_terminators(void)
{
  XsMirError error = {0};
  XsMirModule *module = NULL;
  CHECK(xs_mir_module_create("project", &module, &error) == XS_MIR_OK);
  XsMirFunction *function = NULL;
  CHECK(xs_mir_module_add_function_definition(module, "App.Main", (XsMirType){.kind = XS_LIL_TYPE_I64}, NULL, 0,
                                              &function, &error) == XS_MIR_OK);
  CHECK(function != NULL);
  CHECK(xs_mir_function_is_definition(function));
  XsMirLocalId argument = 0;
  XsMirLocalId state = 0;
  CHECK(xs_mir_function_add_local(function, XS_MIR_LOCAL_PARAMETER, "argc", (XsMirType){.kind = XS_LIL_TYPE_I32}, false,
                                  &argument, &error) == XS_MIR_OK);
  CHECK(xs_mir_function_add_local(function, XS_MIR_LOCAL_VARIABLE, "state", (XsMirType){.kind = XS_LIL_TYPE_I64}, true,
                                  &state, &error) == XS_MIR_OK);
  CHECK(xs_mir_function_local_count(function) == 2);
  CHECK(xs_mir_function_local_kind(function, argument) == XS_MIR_LOCAL_PARAMETER);
  CHECK(strcmp(xs_mir_function_local_name(function, state), "state") == 0);
  CHECK(xs_mir_function_local_type(function, state).kind == XS_LIL_TYPE_I64);
  CHECK(xs_mir_function_local_is_mutable(function, state));
  XsMirPlace *place = NULL;
  CHECK(xs_mir_function_add_local_place(function, state, &place, &error) == XS_MIR_OK);
  CHECK(xs_mir_place_id(place) == 0);
  CHECK(xs_mir_place_root_local(place) == state);
  CHECK(xs_mir_place_add_field_projection(place, 0, &error) == XS_MIR_OK);
  CHECK(xs_mir_place_add_deref_projection(place, &error) == XS_MIR_OK);
  CHECK(xs_mir_place_add_index_projection(place, 17, &error) == XS_MIR_OK);
  CHECK(xs_mir_place_projection_count(place) == 3);
  CHECK(xs_mir_place_projection_kind(place, 0) == XS_MIR_PLACE_PROJECTION_FIELD);
  CHECK(xs_mir_place_projection_kind(place, 1) == XS_MIR_PLACE_PROJECTION_DEREF);
  CHECK(xs_mir_place_projection_kind(place, 2) == XS_MIR_PLACE_PROJECTION_INDEX);
  XsMirBlock *entry = NULL;
  XsMirBlock *done = NULL;
  CHECK(xs_mir_function_append_block(function, "entry", &entry, &error) == XS_MIR_OK);
  CHECK(xs_mir_function_append_block(function, "done", &done, &error) == XS_MIR_OK);
  XsMirValueId constant = 0;
  CHECK(xs_mir_block_add_const_i64(entry, 42, &constant, &error) == XS_MIR_OK);
  CHECK(xs_mir_function_value_type(function, constant).kind == XS_LIL_TYPE_I64);
  CHECK(xs_mir_block_add_store(entry, place, constant, &error) == XS_MIR_OK);
  XsMirValueId loaded = 0;
  CHECK(xs_mir_block_add_load(entry, place, (XsMirType){.kind = XS_LIL_TYPE_I64}, &loaded, &error) == XS_MIR_OK);
  CHECK(xs_mir_function_value_type(function, loaded).kind == XS_LIL_TYPE_I64);
  CHECK(xs_mir_block_instruction_count(entry) == 3);
  CHECK(xs_mir_block_instruction_kind(entry, 0) == XS_MIR_INSTRUCTION_CONST_I64);
  CHECK(xs_mir_block_instruction_kind(entry, 1) == XS_MIR_INSTRUCTION_STORE);
  CHECK(xs_mir_block_instruction_kind(entry, 2) == XS_MIR_INSTRUCTION_LOAD);
  CHECK(xs_mir_function_block_count(function) == 2);
  CHECK(xs_mir_block_id(entry) == 0);
  CHECK(strcmp(xs_mir_block_label(done), "done") == 0);
  CHECK(xs_mir_block_set_goto(entry, done, &error) == XS_MIR_OK);
  CHECK(xs_mir_block_set_return_value(done, loaded, &error) == XS_MIR_OK);
  CHECK(xs_mir_block_terminator_kind(entry) == XS_MIR_TERMINATOR_GOTO);
  CHECK(xs_mir_block_terminator_kind(done) == XS_MIR_TERMINATOR_RETURN);

  FILE *stream = tmpfile();
  if (stream == NULL) {
    ++failures;
    xs_mir_module_destroy(module);
    return;
  }
  CHECK(xs_mir_module_write_text(module, stream, &error) == XS_MIR_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[512] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "define fn App.Main() -> i64 {\n") != NULL);
  CHECK(strstr(buffer, "local 0 param argc: i32\n") != NULL);
  CHECK(strstr(buffer, "local 1 var mut state: i64\n") != NULL);
  CHECK(strstr(buffer, "bb0 entry:\n  v0 = const.i64 42\n  store place0, v0\n  v1 = load place0\n  goto bb1\n") !=
        NULL);
  CHECK(strstr(buffer, "bb1 done:\n  return 1\n") != NULL);
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
  test_function_definition_blocks_and_terminators();
  test_hir_function_declaration_lowering();
  test_xlil_declaration_lowering();
  return failures == 0 ? 0 : 1;
}
