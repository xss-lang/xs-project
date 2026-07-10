/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/backend/linker.h"
#include "xs/backend/llvm_backend.h"

#include <llvm-c/Core.h>

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

static bool file_contains(const char *path, const char *needle)
{
  FILE *file = fopen(path, "rb");
  if (file == nullptr)
    return false;
  char buffer[4096] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1U, file);
  fclose(file);
  buffer[read] = '\0';
  return strstr(buffer, needle) != nullptr;
}

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "backend test requires object path and linker path\n");
    return 2;
  }

  XsBackendError error = {0};
  XsLlvmBackend *backend = nullptr;
  XsLlvmBackendConfig config = {
      .optimization = XS_LLVM_OPT_NONE,
      .verify_modules = true,
  };
  CHECK(xs_llvm_backend_create(&config, &backend, &error) == XS_BACKEND_OK);
  CHECK(backend != nullptr);
  CHECK(xs_llvm_backend_context(backend) != nullptr);
  CHECK(xs_llvm_backend_target_triple(backend) != nullptr && xs_llvm_backend_target_triple(backend)[0] != '\0');
  CHECK(xs_llvm_backend_data_layout(backend) != nullptr && xs_llvm_backend_data_layout(backend)[0] != '\0');

  XsLlvmCodegenUnit *first = nullptr;
  XsLlvmCodegenUnit *second = nullptr;
  CHECK(xs_llvm_codegen_unit_create(backend, "first", &first, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_codegen_unit_create(backend, "second", &second, &error) == XS_BACKEND_OK);
  CHECK(first != nullptr && second != nullptr);
  CHECK(xs_llvm_codegen_unit_module(first) != xs_llvm_codegen_unit_module(second));

  LLVMTypeRef type = nullptr;
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_BOOL, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 1);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_BYTE, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 8);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_SBYTE, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 8);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_INT, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 64);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_INTEGER, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 128);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_CHAR, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 16);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_SFLOAT, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetTypeKind(type) == LLVMFloatTypeKind);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_STR, &type, &error) == XS_BACKEND_DEFERRED);
  CHECK(xs_llvm_lil_type(backend, (XsLilType){.kind = XS_LIL_TYPE_BOOL}, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 1);
  CHECK(xs_llvm_lil_type(backend, (XsLilType){.kind = XS_LIL_TYPE_I64}, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 64);
  CHECK(xs_llvm_lil_type(backend, (XsLilType){.kind = XS_LIL_TYPE_F16}, &type, &error) == XS_BACKEND_DEFERRED);

  const XsPrimitiveType parameters[] = {XS_PRIMITIVE_INT, XS_PRIMITIVE_INT};
  XsFunctionSignature signature = {
      .name = "Add",
      .return_type = XS_PRIMITIVE_INT,
      .parameter_types = parameters,
      .parameter_count = 2,
  };
  LLVMValueRef function = nullptr;
  CHECK(xs_llvm_declare_function(first, &signature, &function, &error) == XS_BACKEND_OK);
  CHECK(function != nullptr);
  CHECK(LLVMCountParamTypes(LLVMGlobalGetValueType(function)) == 2);
  CHECK(LLVMCountBasicBlocks(function) == 0);
  const XsLilType import_parameters[] = {{.kind = XS_LIL_TYPE_I64}};
  CHECK(xs_llvm_declare_lil_function(first, "Import", (XsLilType){.kind = XS_LIL_TYPE_I64}, import_parameters, 1,
                                     &function, &error) == XS_BACKEND_OK);
  CHECK(function != nullptr);
  CHECK(LLVMCountParamTypes(LLVMGlobalGetValueType(function)) == 1);
  CHECK(xs_llvm_declare_lil_function(first, "Sink", (XsLilType){.kind = XS_LIL_TYPE_VOID}, import_parameters, 1,
                                     &function, &error) == XS_BACKEND_OK);
  XsLilModule *lil_module = nullptr;
  CHECK(xs_lil_module_create("Lowering", &lil_module, nullptr) == XS_LIL_OK);
  XsLilFunction *lil_function = nullptr;
  CHECK(xs_lil_module_add_function_definition(lil_module, "Answer", (XsLilType){.kind = XS_LIL_TYPE_I64}, nullptr, 0,
                                              &lil_function, nullptr) == XS_LIL_OK);
  XsLilBlock *lil_entry = nullptr;
  CHECK(xs_lil_function_append_block(lil_function, "entry", &lil_entry, nullptr) == XS_LIL_OK);
  XsLilValueId answer = 0;
  CHECK(xs_lil_block_add_const_i64(lil_entry, 42, &answer, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(lil_entry, answer, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "Answer", (XsLilType){.kind = XS_LIL_TYPE_I64}, nullptr, 0, &function,
                                     &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, xs_lil_module_function_at(lil_module, 0), &error) == XS_BACKEND_OK);
  CHECK(xs_lil_module_add_function(lil_module, "Import", (XsLilType){.kind = XS_LIL_TYPE_I64}, import_parameters, 1,
                                   nullptr) == XS_LIL_OK);
  CHECK(xs_lil_module_add_function(lil_module, "Sink", (XsLilType){.kind = XS_LIL_TYPE_VOID}, import_parameters, 1,
                                   nullptr) == XS_LIL_OK);
  XsLilFunction *identity = nullptr;
  CHECK(xs_lil_module_add_function_definition(lil_module, "Identity", (XsLilType){.kind = XS_LIL_TYPE_I64},
                                              import_parameters, 1, &identity, nullptr) == XS_LIL_OK);
  XsLilBlock *identity_entry = nullptr;
  CHECK(xs_lil_function_append_block(identity, "entry", &identity_entry, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(identity_entry, 0, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "Identity", (XsLilType){.kind = XS_LIL_TYPE_I64}, import_parameters, 1,
                                     &function, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, identity, &error) == XS_BACKEND_OK);
  XsLilFunction *call_import = nullptr;
  CHECK(xs_lil_module_add_function_definition(lil_module, "CallImport", (XsLilType){.kind = XS_LIL_TYPE_I64}, nullptr,
                                              0, &call_import, nullptr) == XS_LIL_OK);
  XsLilBlock *call_import_entry = nullptr;
  CHECK(xs_lil_function_append_block(call_import, "entry", &call_import_entry, nullptr) == XS_LIL_OK);
  XsLilValueId import_argument = 0;
  XsLilValueId import_result = 0;
  CHECK(xs_lil_block_add_const_i64(call_import_entry, 7, &import_argument, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_add_call(call_import_entry, "Import", (XsLilType){.kind = XS_LIL_TYPE_I64}, &import_argument, 1,
                              &import_result, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(call_import_entry, import_result, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "CallImport", (XsLilType){.kind = XS_LIL_TYPE_I64}, nullptr, 0, &function,
                                     &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, call_import, &error) == XS_BACKEND_OK);
  XsLilFunction *call_sink = nullptr;
  CHECK(xs_lil_module_add_function_definition(lil_module, "CallSink", (XsLilType){.kind = XS_LIL_TYPE_VOID}, nullptr, 0,
                                              &call_sink, nullptr) == XS_LIL_OK);
  XsLilBlock *call_sink_entry = nullptr;
  CHECK(xs_lil_function_append_block(call_sink, "entry", &call_sink_entry, nullptr) == XS_LIL_OK);
  XsLilValueId sink_argument = 0;
  CHECK(xs_lil_block_add_const_i64(call_sink_entry, 9, &sink_argument, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_add_void_call(call_sink_entry, "Sink", &sink_argument, 1, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(call_sink_entry, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "CallSink", (XsLilType){.kind = XS_LIL_TYPE_VOID}, nullptr, 0, &function,
                                     &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, call_sink, &error) == XS_BACKEND_OK);
  XsLilFunction *arithmetic = nullptr;
  CHECK(xs_lil_module_add_function_definition(lil_module, "Arithmetic", (XsLilType){.kind = XS_LIL_TYPE_I64},
                                              import_parameters, 1, &arithmetic, nullptr) == XS_LIL_OK);
  XsLilBlock *arithmetic_entry = nullptr;
  CHECK(xs_lil_function_append_block(arithmetic, "entry", &arithmetic_entry, nullptr) == XS_LIL_OK);
  XsLilValueId left = 0;
  XsLilValueId right = 0;
  XsLilValueId sum = 0;
  XsLilValueId difference = 0;
  XsLilValueId product = 0;
  CHECK(xs_lil_block_add_i64(arithmetic_entry, left, right, &sum, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_sub_i64(arithmetic_entry, sum, right, &difference, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_mul_i64(arithmetic_entry, difference, right, &product, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(arithmetic_entry, product, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "Arithmetic", (XsLilType){.kind = XS_LIL_TYPE_I64}, import_parameters, 1,
                                     &function, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, arithmetic, &error) == XS_BACKEND_OK);
  XsLilFunction *equality = nullptr;
  CHECK(xs_lil_module_add_function_definition(lil_module, "Equality", (XsLilType){.kind = XS_LIL_TYPE_BOOL},
                                              import_parameters, 1, &equality, nullptr) == XS_LIL_OK);
  XsLilBlock *equality_entry = nullptr;
  CHECK(xs_lil_function_append_block(equality, "entry", &equality_entry, nullptr) == XS_LIL_OK);
  XsLilValueId equality_left = 0;
  XsLilValueId equality_right = 0;
  XsLilValueId equal = 0;
  CHECK(xs_lil_block_eq_i64(equality_entry, equality_left, equality_right, &equal, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(equality_entry, equal, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "Equality", (XsLilType){.kind = XS_LIL_TYPE_BOOL}, import_parameters, 1,
                                     &function, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, equality, &error) == XS_BACKEND_OK);
  XsLilFunction *branch_function = nullptr;
  const XsLilType bool_parameter[] = {{.kind = XS_LIL_TYPE_BOOL}};
  CHECK(xs_lil_module_add_function_definition(lil_module, "Choose", (XsLilType){.kind = XS_LIL_TYPE_VOID},
                                              bool_parameter, 1, &branch_function, nullptr) == XS_LIL_OK);
  XsLilBlock *branch_entry = nullptr;
  XsLilBlock *branch_then = nullptr;
  XsLilBlock *branch_else = nullptr;
  CHECK(xs_lil_function_append_block(branch_function, "entry", &branch_entry, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(branch_function, "then", &branch_then, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_function_append_block(branch_function, "else", &branch_else, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_branch_if(branch_entry, 0, 1, 2, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(branch_then, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(branch_else, nullptr) == XS_LIL_OK);
  CHECK(xs_lil_module_verify(lil_module, nullptr) == XS_LIL_OK);
  CHECK(xs_llvm_declare_lil_function(first, "Choose", (XsLilType){.kind = XS_LIL_TYPE_VOID}, bool_parameter, 1,
                                     &function, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_lower_lil_function_body(first, branch_function, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_optimize_codegen_unit(first, &error) == XS_BACKEND_OK);
  char ir_path[4096] = {0};
  CHECK(snprintf(ir_path, sizeof(ir_path), "%s.ll", argv[1]) > 0);
  CHECK(xs_llvm_write_ir_file(first, ir_path, &error) == XS_BACKEND_OK);
  CHECK(file_contains(ir_path, "target triple"));
  CHECK(file_contains(ir_path, "declare i64 @Add(i64, i64)"));
  CHECK(file_contains(ir_path, "declare i64 @Import(i64)"));
  CHECK(file_contains(ir_path, "declare void @Sink(i64)"));
  CHECK(file_contains(ir_path, "define i64 @Answer()"));
  CHECK(file_contains(ir_path, "ret i64 42"));
  CHECK(file_contains(ir_path, "define i64 @Identity(i64"));
  CHECK(file_contains(ir_path, "define i64 @CallImport()"));
  CHECK(file_contains(ir_path, "call i64 @Import(i64 7)"));
  CHECK(file_contains(ir_path, "define void @CallSink()"));
  CHECK(file_contains(ir_path, "call void @Sink(i64 9)"));
  CHECK(file_contains(ir_path, "define i64 @Arithmetic(i64"));
  CHECK(file_contains(ir_path, "add i64"));
  CHECK(file_contains(ir_path, "sub i64"));
  CHECK(file_contains(ir_path, "mul i64"));
  CHECK(file_contains(ir_path, "define i1 @Equality(i64"));
  CHECK(file_contains(ir_path, "icmp eq i64"));
  CHECK(file_contains(ir_path, "define void @Choose(i1"));
  CHECK(file_contains(ir_path, "br i1"));
  CHECK(xs_llvm_emit_object_file(first, argv[1], &error) == XS_BACKEND_OK);

  const char *linker_arguments[] = {"--version"};
  XsLinkerInvocation invocation = {
      .program = argv[2],
      .arguments = linker_arguments,
      .argument_count = 1,
  };
  int exit_code = -1;
  CHECK(xs_linker_invoke(&invocation, &exit_code, &error) == XS_BACKEND_OK);
  CHECK(exit_code == 0);

  xs_llvm_codegen_unit_destroy(second);
  xs_llvm_codegen_unit_destroy(first);
  xs_llvm_backend_destroy(backend);
  xs_lil_module_destroy(lil_module);
  remove(ir_path);
  remove(argv[1]);
  return failures == 0 ? 0 : 1;
}
