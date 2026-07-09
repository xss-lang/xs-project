/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/lil.h"
#include "xs/lil/aot.h"

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

static void test_module_and_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.Main", &module, &error) == XS_LIL_OK);
  CHECK(module != NULL);
  CHECK(strcmp(xs_lil_module_name(module), "App.Main") == 0);
  const XsLilType parameters[] = {
      {.kind = XS_LIL_TYPE_U8},
      {.kind = XS_LIL_TYPE_I8},
      {.kind = XS_LIL_TYPE_BOOL},
  };
  CHECK(xs_lil_module_add_function(module, "Check", (XsLilType){.kind = XS_LIL_TYPE_BOOL}, parameters, 3, &error) ==
        XS_LIL_OK);
  CHECK(xs_lil_module_function_count(module) == 1);
  CHECK(strcmp(xs_lil_type_name((XsLilType){.kind = XS_LIL_TYPE_BOOL}), "bool") == 0);
  CHECK(strcmp(xs_lil_type_name((XsLilType){.kind = XS_LIL_TYPE_U8}), "u8") == 0);
  CHECK(strcmp(xs_lil_type_name((XsLilType){.kind = XS_LIL_TYPE_I8}), "i8") == 0);

  FILE *stream = tmpfile();
  if (stream == NULL)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[128] = {0};
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, ".xlil version 0\n") != NULL);
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, ".xlil module App.Main\n") != NULL);
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, ".extern Check : (u8, i8, bool) -> bool\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.Body", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = NULL;
  CHECK(xs_lil_module_add_function_definition(module, "Answer", (XsLilType){.kind = XS_LIL_TYPE_I64}, NULL, 0,
                                              &function, &error) == XS_LIL_OK);
  CHECK(function != NULL);
  XsLilBlock *entry = NULL;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  XsLilValueId value = 0;
  CHECK(xs_lil_block_add_const_i64(entry, 42, &value, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(entry, value, &error) == XS_LIL_OK);

  FILE *stream = tmpfile();
  if (stream == NULL)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[256] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, ".func Answer : () -> i64\n") != NULL);
  CHECK(strstr(buffer, "bb0.entry:\n  %0:i64 = const 42\n  ret %0\n.end\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_rejects_missing_return_value(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.BadBody", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = NULL;
  CHECK(xs_lil_module_add_function_definition(module, "Bad", (XsLilType){.kind = XS_LIL_TYPE_I64}, NULL, 0, &function,
                                              &error) == XS_LIL_OK);
  XsLilBlock *entry = NULL;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(entry, &error) == XS_LIL_INVALID_ARGUMENT);
  xs_lil_module_destroy(module);
}

static void test_function_body_branch_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.Branch", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = NULL;
  CHECK(xs_lil_module_add_function_definition(module, "Jump", (XsLilType){.kind = XS_LIL_TYPE_VOID}, NULL, 0, &function,
                                              &error) == XS_LIL_OK);
  XsLilBlock *entry = NULL;
  XsLilBlock *exit = NULL;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(function, "exit", &exit, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_branch(entry, 1, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(exit, &error) == XS_LIL_OK);

  FILE *stream = tmpfile();
  if (stream == NULL)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[256] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "bb0.entry:\n  br bb1\nbb1.exit:\n  ret\n.end\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_reads_external_signature(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.extern Import : (i64) -> i64\n";
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_parse_text("extern.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(module != NULL);
  CHECK(strcmp(xs_lil_module_name(module), "App") == 0);
  CHECK(xs_lil_module_function_count(module) == 1);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(function != NULL);
  CHECK(strcmp(xs_lil_function_name(function), "Import") == 0);
  CHECK(!xs_lil_function_is_definition(function));
  CHECK(xs_lil_function_return_type(function).kind == XS_LIL_TYPE_I64);
  CHECK(xs_lil_function_parameter_count(function) == 1);
  CHECK(xs_lil_function_parameter_type(function, 0).kind == XS_LIL_TYPE_I64);
  xs_lil_module_destroy(module);
}

static void test_text_parser_reads_function_definition(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Main : () -> void\nbb0.entry:\n  ret\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_parse_text("func.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(function != NULL);
  CHECK(strcmp(xs_lil_function_name(function), "Main") == 0);
  CHECK(xs_lil_function_is_definition(function));
  CHECK(xs_lil_function_return_type(function).kind == XS_LIL_TYPE_VOID);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_supported_body_subset(void)
{
  const char text[] =
      ".xlil version 0\n.xlil module App\n.func Answer : () -> i64\nbb0.entry:\n  %0:i64 = const 42\n  ret %0\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_parse_text("body.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if (stream == NULL)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[256] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, ".func Answer : () -> i64\n") != NULL);
  CHECK(strstr(buffer, "bb0.entry:\n  %0:i64 = const 42\n  ret %0\n.end\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_branch_subset(void)
{
  const char text[] =
      ".xlil version 0\n.xlil module App\n.func Jump : () -> void\nbb0.entry:\n  br bb1\nbb1.exit:\n  ret\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_parse_text("branch.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if (stream == NULL)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[256] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "bb0.entry:\n  br bb1\nbb1.exit:\n  ret\n.end\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_rejects_invalid_inputs(void)
{
  static const char *const invalid_inputs[] = {
      ".xlil version 1\n.xlil module App\n",
      ".xlil version 0\n.extern Import : () -> void\n",
      ".xlil version 0\n.xlil module App\n.extern Import : (bad) -> void\n",
      ".xlil version 0\n.xlil module App\n.extern Import (i64) -> i64\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> void\nbb0.entry:\n  ret\n  %0:i64 = const 1\n.end\n",
  };
  for (size_t i = 0; i < sizeof(invalid_inputs) / sizeof(invalid_inputs[0]); ++i)
  {
    XsLilError error = {0};
    XsLilModule *module = NULL;
    CHECK(xs_lil_module_parse_text("bad.xlil", invalid_inputs[i], strlen(invalid_inputs[i]), &module, &error) ==
          XS_LIL_INVALID_ARGUMENT);
    CHECK(module == NULL);
  }
}

int main(void)
{
  test_module_and_text_writer();
  test_function_body_text_writer();
  test_function_body_rejects_missing_return_value();
  test_function_body_branch_text_writer();
  test_text_parser_reads_external_signature();
  test_text_parser_reads_function_definition();
  test_text_parser_round_trips_supported_body_subset();
  test_text_parser_round_trips_branch_subset();
  test_text_parser_rejects_invalid_inputs();
  return failures == 0 ? 0 : 1;
}
