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
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while(0)

static void test_module_and_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_create("App.Main", &module, &error) == XS_LIL_OK);
  CHECK(module != nullptr);
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
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[128] = {0};
  CHECK(fgets(buffer, sizeof(buffer), stream) != nullptr);
  CHECK(strstr(buffer, ".xlil version 0\n") != nullptr);
  CHECK(fgets(buffer, sizeof(buffer), stream) != nullptr);
  CHECK(strstr(buffer, ".xlil module App.Main\n") != nullptr);
  CHECK(fgets(buffer, sizeof(buffer), stream) != nullptr);
  CHECK(strstr(buffer, ".extern Check : (u8, i8, bool) -> bool\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_create("App.Body", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = nullptr;
  CHECK(xs_lil_module_add_function_definition(module, "Answer", (XsLilType){.kind = XS_LIL_TYPE_I64}, nullptr, 0,
                                              &function, &error) == XS_LIL_OK);
  CHECK(function != nullptr);
  XsLilBlock *entry = nullptr;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  XsLilValueId value = 0;
  CHECK(xs_lil_block_add_const_i64(entry, 42, &value, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(entry, value, &error) == XS_LIL_OK);

  FILE *stream = tmpfile();
  if(stream == nullptr)
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
  CHECK(strstr(buffer, ".func Answer : () -> i64\n") != nullptr);
  CHECK(strstr(buffer, "bb0.entry:\n  %r0:i64 = const 42\n  ret %r0\n.end\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_rejects_missing_return_value(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_create("App.BadBody", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = nullptr;
  CHECK(xs_lil_module_add_function_definition(module, "Bad", (XsLilType){.kind = XS_LIL_TYPE_I64}, nullptr, 0,
                                              &function, &error) == XS_LIL_OK);
  XsLilBlock *entry = nullptr;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(entry, &error) == XS_LIL_INVALID_ARGUMENT);
  xs_lil_module_destroy(module);
}

static void test_floating_constant_bits(void)
{
  const char *text = ".xlil version 0\n.xlil module FloatConstants\n"
                     ".func F32Value : () -> f32\nbb0.entry:\n"
                     "  %r0:f32 = const.f32 0x3fc00000\n  ret %r0\n.end\n"
                     ".func F64Value : () -> f64\nbb0.entry:\n"
                     "  %r0:f64 = const.f64 0x3ff8000000000000\n  ret %r0\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("FloatConstants.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(module != nullptr);
  CHECK(xs_lil_module_verify(module, &error) == XS_LIL_OK);
  const XsLilFunction *f32 = xs_lil_module_function_at(module, 0);
  const XsLilFunction *f64 = xs_lil_module_function_at(module, 1);
  CHECK(xs_lil_block_instruction_kind(xs_lil_function_block_at(f32, 0), 0) == XS_LIL_INSTRUCTION_CONST_F32);
  CHECK(xs_lil_block_instruction_float_bits(xs_lil_function_block_at(f32, 0), 0) == UINT32_C(0x3fc00000));
  CHECK(xs_lil_block_instruction_kind(xs_lil_function_block_at(f64, 0), 0) == XS_LIL_INSTRUCTION_CONST_F64);
  CHECK(xs_lil_block_instruction_float_bits(xs_lil_function_block_at(f64, 0), 0) == UINT64_C(0x3ff8000000000000));
  xs_lil_module_destroy(module);
}

static void test_function_body_branch_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_create("App.Branch", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = nullptr;
  CHECK(xs_lil_module_add_function_definition(module, "Jump", (XsLilType){.kind = XS_LIL_TYPE_VOID}, nullptr, 0,
                                              &function, &error) == XS_LIL_OK);
  XsLilBlock *entry = nullptr;
  XsLilBlock *exit = nullptr;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(function, "exit", &exit, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_branch(entry, 1, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(exit, &error) == XS_LIL_OK);

  FILE *stream = tmpfile();
  if(stream == nullptr)
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
  CHECK(strstr(buffer, "bb0.entry:\n  br bb1\nbb1.exit:\n  ret\n.end\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_branch_if_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_create("App.BranchIf", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = nullptr;
  CHECK(xs_lil_module_add_function_definition(module, "Choose", (XsLilType){.kind = XS_LIL_TYPE_VOID}, nullptr, 0,
                                              &function, &error) == XS_LIL_OK);
  XsLilBlock *entry = nullptr;
  XsLilBlock *then_block = nullptr;
  XsLilBlock *else_block = nullptr;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(function, "then", &then_block, &error) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(function, "else", &else_block, &error) == XS_LIL_OK);
  XsLilValueId condition = 0;
  CHECK(xs_lil_block_add_const_bool(entry, true, &condition, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_branch_if(entry, condition, 1, 2, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(then_block, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(else_block, &error) == XS_LIL_OK);

  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[1024] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "bb0.entry:\n  %r0:bool = const.bool true\n  br_if %r0, bb1, bb2\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_rejects_non_bool_branch_if(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_create("App.BadBranchIf", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = nullptr;
  CHECK(xs_lil_module_add_function_definition(module, "Bad", (XsLilType){.kind = XS_LIL_TYPE_VOID}, nullptr, 0,
                                              &function, &error) == XS_LIL_OK);
  XsLilBlock *entry = nullptr;
  XsLilBlock *then_block = nullptr;
  XsLilBlock *else_block = nullptr;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(function, "then", &then_block, &error) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(function, "else", &else_block, &error) == XS_LIL_OK);
  XsLilValueId condition = 0;
  CHECK(xs_lil_block_add_const_i64(entry, 1, &condition, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_branch_if(entry, condition, 1, 2, &error) == XS_LIL_INVALID_ARGUMENT);
  xs_lil_module_destroy(module);
}

static void test_text_parser_reads_external_signature(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.extern Import : (i64) -> i64\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("extern.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(module != nullptr);
  CHECK(strcmp(xs_lil_module_name(module), "App") == 0);
  CHECK(xs_lil_module_function_count(module) == 1);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(function != nullptr);
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
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("func.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(function != nullptr);
  CHECK(strcmp(xs_lil_function_name(function), "Main") == 0);
  CHECK(xs_lil_function_is_definition(function));
  CHECK(xs_lil_function_return_type(function).kind == XS_LIL_TYPE_VOID);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_supported_body_subset(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Answer : () -> i64\nbb0.entry:\n  %r0:i64 = const 42\n "
                      " ret %r0\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("body.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if(stream == nullptr)
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
  CHECK(strstr(buffer, ".func Answer : () -> i64\n") != nullptr);
  CHECK(strstr(buffer, "bb0.entry:\n  %r0:i64 = const 42\n  ret %r0\n.end\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_branch_subset(void)
{
  const char text[] =
      ".xlil version 0\n.xlil module App\n.func Jump : () -> void\nbb0.entry:\n  br bb1\nbb1.exit:\n  ret\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("branch.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if(stream == nullptr)
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
  CHECK(strstr(buffer, "bb0.entry:\n  br bb1\nbb1.exit:\n  ret\n.end\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_branch_if_subset(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Choose : () -> void\nbb0.entry:\n  %r0:bool = "
                      "const.bool true\n  br_if %r0, bb1, bb2\nbb1.then:\n  ret\nbb2.else:\n  ret\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("branch_if.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[1024] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "br_if %r0, bb1, bb2\nbb1.then:\n  ret\nbb2.else:\n  ret\n.end\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_bool_not_subset(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Flip : () -> bool\nbb0.entry:\n"
                      "  %r0:bool = const.bool false\n  %r1:bool = not.bool %r0\n  ret %r1\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("not_bool.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(module != nullptr);
  CHECK(xs_lil_module_verify(module, &error) == XS_LIL_OK);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(function != nullptr);
  const XsLilBlock *block = xs_lil_function_block_at(function, 0);
  CHECK(block != nullptr);
  CHECK(xs_lil_block_instruction_kind(block, 1) == XS_LIL_INSTRUCTION_NOT_BOOL);
  CHECK(xs_lil_function_value_type(function, 1).kind == XS_LIL_TYPE_BOOL);
  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[512] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1U, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "%r1:bool = not.bool %r0\n  ret %r1\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_parameters_and_calls(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Forward : (i64) -> i64\n.param %r0:i64\nbb0.entry:\n"
                      "  %r1:i64 = call Import(%r0)\n  call Sink(%r1)\n  ret %r1\n.end\n.extern Import : (i64) -> i64\n"
                      ".extern Sink : (i64) -> void\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("calls.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(module != nullptr);
  CHECK(xs_lil_module_verify(module, &error) == XS_LIL_OK);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(function != nullptr);
  CHECK(xs_lil_function_value_count(function) == 2);
  CHECK(xs_lil_function_value_type(function, 0).kind == XS_LIL_TYPE_I64);
  const XsLilBlock *block = xs_lil_function_block_at(function, 0);
  CHECK(block != nullptr);
  CHECK(xs_lil_block_instruction_kind(block, 0) == XS_LIL_INSTRUCTION_CALL);
  CHECK(strcmp(xs_lil_block_instruction_callee(block, 0), "Import") == 0);
  CHECK(xs_lil_block_instruction_argument_count(block, 0) == 1);
  CHECK(xs_lil_block_instruction_argument(block, 0, 0) == 0);
  CHECK(xs_lil_block_instruction_result(block, 1) == UINT32_MAX);
  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[1024] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, ".param %r0:i64\n") != nullptr);
  CHECK(strstr(buffer, "%r1:i64 = call Import(%r0)\n  call Sink(%r1)\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_panic_terminator(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Fail : () -> void\nbb0.entry:\n  panic\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("panic.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(module != nullptr);
  CHECK(xs_lil_module_verify(module, &error) == XS_LIL_OK);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  const XsLilBlock *block = xs_lil_function_block_at(function, 0);
  CHECK(xs_lil_block_terminator_kind(block) == XS_LIL_TERMINATOR_PANIC);
  FILE *stream = tmpfile();
  CHECK(stream != nullptr);
  if(stream != nullptr)
  {
    CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    char buffer[256] = {0};
    size_t read = fread(buffer, 1, sizeof(buffer) - 1U, stream);
    buffer[read] = '\0';
    CHECK(strstr(buffer, "bb0.entry:\n  panic\n.end\n") != nullptr);
    fclose(stream);
  }
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_stack_slots(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Memory : () -> i32\n.slot %s0:i32\n"
                      "bb0.entry:\n  %r0:i32 = const.i32 7\n  store %r0, %s0\n"
                      "  %r1:i32 = load %s0\n  ret %r1\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("memory.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  CHECK(xs_lil_module_verify(module, &error) == XS_LIL_OK);
  const XsLilFunction *function = xs_lil_module_function_at(module, 0);
  CHECK(xs_lil_function_slot_count(function) == 1);
  CHECK(xs_lil_function_slot_type(function, 0).kind == XS_LIL_TYPE_I32);
  const XsLilBlock *block = xs_lil_function_block_at(function, 0);
  CHECK(xs_lil_block_instruction_kind(block, 1) == XS_LIL_INSTRUCTION_STORE);
  CHECK(xs_lil_block_instruction_kind(block, 2) == XS_LIL_INSTRUCTION_LOAD);
  CHECK(xs_lil_block_instruction_slot(block, 1) == 0);
  CHECK(xs_lil_block_instruction_slot(block, 2) == 0);
  FILE *stream = tmpfile();
  CHECK(stream != nullptr);
  if(stream != nullptr)
  {
    CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
    CHECK(fseek(stream, 0, SEEK_SET) == 0);
    char buffer[512] = {0};
    size_t read = fread(buffer, 1, sizeof(buffer) - 1U, stream);
    buffer[read] = '\0';
    CHECK(strstr(buffer, ".slot %s0:i32\n") != nullptr);
    CHECK(strstr(buffer, "store %r0, %s0\n  %r1:i32 = load %s0\n") != nullptr);
    fclose(stream);
  }
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_binary_i64_instructions(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Arithmetic : () -> i64\nbb0.entry:\n"
                      "  %r0:i64 = const 9\n  %r1:i64 = const 3\n  %r2:i64 = add.i64 %r0, %r1\n"
                      "  %r3:i64 = sub.i64 %r2, %r1\n  %r4:i64 = mul.i64 %r3, %r1\n"
                      "  %r5:i64 = div.i64 %r4, %r1\n  %r6:i64 = rem.i64 %r5, %r1\n"
                      "  %r7:i64 = and.i64 %r0, %r1\n  %r8:i64 = or.i64 %r0, %r1\n"
                      "  %r9:i64 = xor.i64 %r7, %r8\n  %r10:i64 = shl.i64 %r1, %r1\n"
                      "  %r11:i64 = shr.i64 %r0, %r1\n  %r12:bool = eq.i64 %r6, %r0\n"
                      "  %r13:bool = ne.i64 %r6, %r0\n  %r14:bool = lt.i64 %r1, %r0\n"
                      "  %r15:bool = le.i64 %r1, %r0\n  %r16:bool = gt.i64 %r0, %r1\n"
                      "  %r17:bool = ge.i64 %r0, %r1\n"
                      "  ret %r6\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("arithmetic.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[1024] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "%r2:i64 = add.i64 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r3:i64 = sub.i64 %r2, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r4:i64 = mul.i64 %r3, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r5:i64 = div.i64 %r4, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r6:i64 = rem.i64 %r5, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r7:i64 = and.i64 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r8:i64 = or.i64 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r9:i64 = xor.i64 %r7, %r8\n") != nullptr);
  CHECK(strstr(buffer, "%r10:i64 = shl.i64 %r1, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r11:i64 = shr.i64 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r12:bool = eq.i64 %r6, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r13:bool = ne.i64 %r6, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r14:bool = lt.i64 %r1, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r15:bool = le.i64 %r1, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r16:bool = gt.i64 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r17:bool = ge.i64 %r0, %r1\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_i32_constant(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func main : () -> i32\nbb0.entry:\n"
                      "  %r0:i32 = const.i32 0\n  ret %r0\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("main.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[256] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1U, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "%r0:i32 = const.i32 0\n") != nullptr);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_text_parser_round_trips_binary_i32_instructions(void)
{
  const char text[] = ".xlil version 0\n.xlil module App\n.func Arithmetic32 : () -> i32\nbb0.entry:\n"
                      "  %r0:i32 = const.i32 9\n  %r1:i32 = const.i32 3\n  %r2:i32 = add.i32 %r0, %r1\n"
                      "  %r3:i32 = sub.i32 %r2, %r1\n  %r4:i32 = mul.i32 %r3, %r1\n"
                      "  %r5:i32 = div.i32 %r4, %r1\n  %r6:i32 = rem.i32 %r5, %r1\n"
                      "  %r7:i32 = and.i32 %r0, %r1\n  %r8:i32 = or.i32 %r7, %r1\n"
                      "  %r9:i32 = xor.i32 %r7, %r8\n  %r10:i32 = shl.i32 %r9, %r1\n"
                      "  %r11:i32 = shr.i32 %r10, %r1\n  %r12:bool = eq.i32 %r6, %r0\n"
                      "  %r13:bool = ne.i32 %r6, %r1\n  %r14:bool = lt.i32 %r1, %r0\n"
                      "  %r15:bool = le.i32 %r1, %r0\n  %r16:bool = gt.i32 %r0, %r1\n"
                      "  %r17:bool = ge.i32 %r0, %r1\n  ret %r11\n.end\n";
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  CHECK(xs_lil_module_parse_text("arithmetic32.xlil", text, strlen(text), &module, &error) == XS_LIL_OK);
  FILE *stream = tmpfile();
  if(stream == nullptr)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[1024] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1U, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "%r2:i32 = add.i32 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r3:i32 = sub.i32 %r2, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r4:i32 = mul.i32 %r3, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r5:i32 = div.i32 %r4, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r6:i32 = rem.i32 %r5, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r7:i32 = and.i32 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r8:i32 = or.i32 %r7, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r9:i32 = xor.i32 %r7, %r8\n") != nullptr);
  CHECK(strstr(buffer, "%r10:i32 = shl.i32 %r9, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r11:i32 = shr.i32 %r10, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r12:bool = eq.i32 %r6, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r13:bool = ne.i32 %r6, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r14:bool = lt.i32 %r1, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r15:bool = le.i32 %r1, %r0\n") != nullptr);
  CHECK(strstr(buffer, "%r16:bool = gt.i32 %r0, %r1\n") != nullptr);
  CHECK(strstr(buffer, "%r17:bool = ge.i32 %r0, %r1\n") != nullptr);
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
      ".xlil version 0\n.xlil module App\n.func Bad : () -> void\nbb0.entry:\n  ret\n  %r0:i64 = const 1\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> i64\nbb0.entry:\n  %0:i64 = const 1\n  ret %r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> i64\nbb0.entry:\n  %r0:i64 = const 1\n  ret %0\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : (i64) -> i64\nbb0.entry:\n  ret %r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : (i64) -> i64\n.param %r1:i64\nbb0.entry:\n  ret %r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> i64\nbb0.entry:\n  %r0:i64 = call Missing()\n  ret "
      "%r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.extern Import : (i64) -> i64\n.func Bad : () -> i64\nbb0.entry:\n"
      "  %r0:i64 = call Import()\n  ret %r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.extern Sink : () -> void\n.func Bad : () -> void\nbb0.entry:\n"
      "  %r0:void = call Sink()\n  ret\n.end\n",
      ".xlil version 0\n.xlil module App\n.extern Import : () -> i64\n.func Bad : () -> void\nbb0.entry:\n"
      "  call Import()\n  ret\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> bool\nbb0.entry:\n"
      "  %r0:i64 = const 1\n  %r1:bool = not.bool %r0\n  ret %r1\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> f32\nbb0.entry:\n"
      "  %r0:f32 = const.f32 0x1234\n  ret %r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> void\n.slot %s0:void\nbb0.entry:\n  ret\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> i64\n.slot %s0:i32\nbb0.entry:\n"
      "  %r0:i64 = load %s0\n  ret %r0\n.end\n",
      ".xlil version 0\n.xlil module App\n.func Bad : () -> void\n.slot %s0:i32\nbb0.entry:\n"
      "  %r0:i64 = const 1\n  store %r0, %s0\n  ret\n.end\n",
  };
  for(size_t i = 0; i < sizeof(invalid_inputs) / sizeof(invalid_inputs[0]); ++i)
  {
    XsLilError error = {0};
    XsLilModule *module = nullptr;
    CHECK(xs_lil_module_parse_text("bad.xlil", invalid_inputs[i], strlen(invalid_inputs[i]), &module, &error) ==
          XS_LIL_INVALID_ARGUMENT);
    CHECK(module == nullptr);
  }
}

int main(void)
{
  test_module_and_text_writer();
  test_function_body_text_writer();
  test_function_body_rejects_missing_return_value();
  test_floating_constant_bits();
  test_function_body_branch_text_writer();
  test_function_body_branch_if_text_writer();
  test_function_body_rejects_non_bool_branch_if();
  test_text_parser_reads_external_signature();
  test_text_parser_reads_function_definition();
  test_text_parser_round_trips_supported_body_subset();
  test_text_parser_round_trips_branch_subset();
  test_text_parser_round_trips_branch_if_subset();
  test_text_parser_round_trips_bool_not_subset();
  test_text_parser_round_trips_parameters_and_calls();
  test_text_parser_round_trips_panic_terminator();
  test_text_parser_round_trips_stack_slots();
  test_text_parser_round_trips_binary_i64_instructions();
  test_text_parser_round_trips_i32_constant();
  test_text_parser_round_trips_binary_i32_instructions();
  test_text_parser_rejects_invalid_inputs();
  return failures == 0 ? 0 : 1;
}
