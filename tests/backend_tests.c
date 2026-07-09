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
  CHECK(xs_llvm_optimize_codegen_unit(first, &error) == XS_BACKEND_OK);
  char ir_path[4096] = {0};
  CHECK(snprintf(ir_path, sizeof(ir_path), "%s.ll", argv[1]) > 0);
  CHECK(xs_llvm_write_ir_file(first, ir_path, &error) == XS_BACKEND_OK);
  CHECK(file_contains(ir_path, "target triple"));
  CHECK(file_contains(ir_path, "declare i64 @Add(i64, i64)"));
  CHECK(file_contains(ir_path, "declare i64 @Import(i64)"));
  CHECK(file_contains(ir_path, "define i64 @Answer()"));
  CHECK(file_contains(ir_path, "ret i64 42"));
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
