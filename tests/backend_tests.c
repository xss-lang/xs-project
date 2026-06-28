#include "xs/backend/linker.h"
#include "xs/backend/llvm_backend.h"

#include <llvm-c/Core.h>

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

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "backend test requires object path and linker path\n");
    return 2;
  }

  XsBackendError error = {0};
  XsLlvmBackend *backend = NULL;
  XsLlvmBackendConfig config = {
      .optimization = XS_LLVM_OPT_NONE,
      .verify_modules = true,
  };
  CHECK(xs_llvm_backend_create(&config, &backend, &error) == XS_BACKEND_OK);
  CHECK(backend != NULL);
  CHECK(xs_llvm_backend_context(backend) != NULL);
  CHECK(xs_llvm_backend_target_triple(backend) != NULL && xs_llvm_backend_target_triple(backend)[0] != '\0');
  CHECK(xs_llvm_backend_data_layout(backend) != NULL && xs_llvm_backend_data_layout(backend)[0] != '\0');

  XsLlvmCodegenUnit *first = NULL;
  XsLlvmCodegenUnit *second = NULL;
  CHECK(xs_llvm_codegen_unit_create(backend, "first", &first, &error) == XS_BACKEND_OK);
  CHECK(xs_llvm_codegen_unit_create(backend, "second", &second, &error) == XS_BACKEND_OK);
  CHECK(first != NULL && second != NULL);
  CHECK(xs_llvm_codegen_unit_module(first) != xs_llvm_codegen_unit_module(second));

  LLVMTypeRef type = NULL;
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_BOOL, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 1);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_BYTE, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 8);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_SBYTE, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 8);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_INT, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 64);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_INT128, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 128);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_CHAR, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetIntTypeWidth(type) == 16);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_FLOAT32, &type, &error) == XS_BACKEND_OK);
  CHECK(LLVMGetTypeKind(type) == LLVMFloatTypeKind);
  CHECK(xs_llvm_primitive_type(backend, XS_PRIMITIVE_STR, &type, &error) == XS_BACKEND_DEFERRED);

  const XsPrimitiveType parameters[] = {XS_PRIMITIVE_INT, XS_PRIMITIVE_INT};
  XsFunctionSignature signature = {
      .name = "Add",
      .return_type = XS_PRIMITIVE_INT,
      .parameter_types = parameters,
      .parameter_count = 2,
  };
  LLVMValueRef function = NULL;
  CHECK(xs_llvm_declare_function(first, &signature, &function, &error) == XS_BACKEND_OK);
  CHECK(function != NULL);
  CHECK(LLVMCountParamTypes(LLVMGlobalGetValueType(function)) == 2);
  CHECK(LLVMCountBasicBlocks(function) == 0);
  CHECK(xs_llvm_optimize_codegen_unit(first, &error) == XS_BACKEND_OK);
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
  remove(argv[1]);
  return failures == 0 ? 0 : 1;
}
