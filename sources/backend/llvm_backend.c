#include "xs/backend/llvm_backend.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/PassBuilder.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct XsLlvmBackend
{
  LLVMContextRef context;
  LLVMTargetMachineRef target_machine;
  LLVMTargetDataRef target_data;
  char *target_triple;
  char *data_layout;
  XsLlvmOptimizationLevel optimization;
  bool verify_modules;
};

struct XsLlvmCodegenUnit
{
  XsLlvmBackend *backend;
  LLVMModuleRef module;
};

static void clear_error(XsBackendError *error)
{
  if (error != NULL)
    *error = (XsBackendError){.status = XS_BACKEND_OK};
}

static XsBackendStatus set_error(XsBackendError *error, XsBackendStatus status, const char *message)
{
  if (error != NULL) {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == NULL ? "unknown backend error" : message);
  }
  return status;
}

static char *copy_text(const char *text)
{
  size_t length = strlen(text);
  char *copy = malloc(length + 1);
  if (copy != NULL)
    memcpy(copy, text, length + 1);
  return copy;
}

static LLVMCodeGenOptLevel codegen_level(XsLlvmOptimizationLevel level)
{
  switch (level) {
  case XS_LLVM_OPT_NONE:
    return LLVMCodeGenLevelNone;
  case XS_LLVM_OPT_LESS:
    return LLVMCodeGenLevelLess;
  case XS_LLVM_OPT_DEFAULT:
    return LLVMCodeGenLevelDefault;
  case XS_LLVM_OPT_AGGRESSIVE:
    return LLVMCodeGenLevelAggressive;
  }
  return LLVMCodeGenLevelDefault;
}

XsBackendStatus xs_llvm_backend_create(const XsLlvmBackendConfig *config, XsLlvmBackend **backend,
                                       XsBackendError *error)
{
  clear_error(error);
  if (config == NULL || backend == NULL)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "backend configuration and output are required");
  if ((unsigned)config->optimization > (unsigned)XS_LLVM_OPT_AGGRESSIVE)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "invalid LLVM optimization level");
  *backend = NULL;

  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmPrinters();

  char *default_triple = NULL;
  const char *requested_triple = config->target_triple;
  if (requested_triple == NULL || requested_triple[0] == '\0') {
    default_triple = LLVMGetDefaultTargetTriple();
    requested_triple = default_triple;
  }

  LLVMTargetRef target = NULL;
  char *llvm_error = NULL;
  if (LLVMGetTargetFromTriple(requested_triple, &target, &llvm_error) != 0) {
    XsBackendStatus status = set_error(error, XS_BACKEND_LLVM_ERROR, llvm_error);
    LLVMDisposeMessage(llvm_error);
    if (default_triple != NULL)
      LLVMDisposeMessage(default_triple);
    return status;
  }

  XsLlvmBackend *result = calloc(1, sizeof(*result));
  if (result == NULL) {
    if (default_triple != NULL)
      LLVMDisposeMessage(default_triple);
    return set_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while creating LLVM backend");
  }
  result->context = LLVMContextCreate();
  result->optimization = config->optimization;
  result->verify_modules = config->verify_modules;
  result->target_triple = copy_text(requested_triple);
  result->target_machine =
      LLVMCreateTargetMachine(target, requested_triple, config->cpu == NULL ? "" : config->cpu,
                              config->features == NULL ? "" : config->features, codegen_level(config->optimization),
                              LLVMRelocDefault, LLVMCodeModelDefault);
  if (default_triple != NULL)
    LLVMDisposeMessage(default_triple);

  if (result->context == NULL || result->target_triple == NULL || result->target_machine == NULL) {
    xs_llvm_backend_destroy(result);
    return set_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not create the target context or target machine");
  }
  result->target_data = LLVMCreateTargetDataLayout(result->target_machine);
  if (result->target_data == NULL) {
    xs_llvm_backend_destroy(result);
    return set_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not determine target data layout");
  }
  char *layout = LLVMCopyStringRepOfTargetData(result->target_data);
  if (layout == NULL) {
    xs_llvm_backend_destroy(result);
    return set_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not format target data layout");
  }
  result->data_layout = copy_text(layout);
  LLVMDisposeMessage(layout);
  if (result->target_data == NULL || result->data_layout == NULL) {
    xs_llvm_backend_destroy(result);
    return set_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not determine target data layout");
  }
  *backend = result;
  return XS_BACKEND_OK;
}

void xs_llvm_backend_destroy(XsLlvmBackend *backend)
{
  if (backend == NULL)
    return;
  if (backend->target_data != NULL)
    LLVMDisposeTargetData(backend->target_data);
  if (backend->target_machine != NULL)
    LLVMDisposeTargetMachine(backend->target_machine);
  if (backend->context != NULL)
    LLVMContextDispose(backend->context);
  free(backend->target_triple);
  free(backend->data_layout);
  free(backend);
}

LLVMContextRef xs_llvm_backend_context(const XsLlvmBackend *backend)
{
  return backend == NULL ? NULL : backend->context;
}

const char *xs_llvm_backend_target_triple(const XsLlvmBackend *backend)
{
  return backend == NULL ? NULL : backend->target_triple;
}

const char *xs_llvm_backend_data_layout(const XsLlvmBackend *backend)
{
  return backend == NULL ? NULL : backend->data_layout;
}

XsBackendStatus xs_llvm_codegen_unit_create(XsLlvmBackend *backend, const char *name, XsLlvmCodegenUnit **unit,
                                            XsBackendError *error)
{
  clear_error(error);
  if (backend == NULL || name == NULL || unit == NULL)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "backend, codegen-unit name, and output are required");
  *unit = calloc(1, sizeof(**unit));
  if (*unit == NULL)
    return set_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while creating codegen unit");
  (*unit)->backend = backend;
  (*unit)->module = LLVMModuleCreateWithNameInContext(name, backend->context);
  if ((*unit)->module == NULL) {
    free(*unit);
    *unit = NULL;
    return set_error(error, XS_BACKEND_LLVM_ERROR, "LLVM could not create a module for the codegen unit");
  }
  LLVMSetTarget((*unit)->module, backend->target_triple);
  LLVMSetDataLayout((*unit)->module, backend->data_layout);
  return XS_BACKEND_OK;
}

void xs_llvm_codegen_unit_destroy(XsLlvmCodegenUnit *unit)
{
  if (unit == NULL)
    return;
  if (unit->module != NULL)
    LLVMDisposeModule(unit->module);
  free(unit);
}

LLVMModuleRef xs_llvm_codegen_unit_module(const XsLlvmCodegenUnit *unit)
{
  return unit == NULL ? NULL : unit->module;
}

XsBackendStatus xs_llvm_primitive_type(XsLlvmBackend *backend, XsPrimitiveType primitive, LLVMTypeRef *type,
                                       XsBackendError *error)
{
  clear_error(error);
  if (backend == NULL || type == NULL)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "backend and LLVM type output are required");
  *type = NULL;
  switch (primitive) {
  case XS_PRIMITIVE_UNIT:
    *type = LLVMVoidTypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_BYTE:
    *type = LLVMInt8TypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_INT16:
  case XS_PRIMITIVE_UINT16:
  case XS_PRIMITIVE_CHAR:
    *type = LLVMInt16TypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_INT32:
  case XS_PRIMITIVE_UINT32:
    *type = LLVMInt32TypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_INT:
  case XS_PRIMITIVE_UINT:
    *type = LLVMInt64TypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_INT128:
  case XS_PRIMITIVE_UINT128:
    *type = LLVMInt128TypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_FLOAT16:
    *type = LLVMHalfTypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_FLOAT32:
    *type = LLVMFloatTypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_FLOAT:
    *type = LLVMDoubleTypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_DOUBLE:
    *type = LLVMFP128TypeInContext(backend->context);
    break;
  case XS_PRIMITIVE_BOOL:
    return set_error(error, XS_BACKEND_DEFERRED,
                     "bool lowering is deferred until nil representation is defined by typed MIR");
  case XS_PRIMITIVE_STR:
    return set_error(error, XS_BACKEND_DEFERRED,
                     "str lowering is deferred until the UTF-16 runtime value layout is documented");
  default:
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "unknown X# primitive type");
  }
  return XS_BACKEND_OK;
}

XsBackendStatus xs_llvm_declare_function(XsLlvmCodegenUnit *unit, const XsFunctionSignature *signature,
                                         LLVMValueRef *function, XsBackendError *error)
{
  clear_error(error);
  if (unit == NULL || signature == NULL || signature->name == NULL || function == NULL ||
      (signature->parameter_count != 0 && signature->parameter_types == NULL))
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "valid codegen unit and function signature are required");
  if (signature->parameter_count > (size_t)UINT_MAX)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "function has too many parameters for LLVM C API");
  if (LLVMGetNamedFunction(unit->module, signature->name) != NULL)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "function is already declared in this codegen unit");

  LLVMTypeRef return_type = NULL;
  XsBackendStatus status = xs_llvm_primitive_type(unit->backend, signature->return_type, &return_type, error);
  if (status != XS_BACKEND_OK)
    return status;
  LLVMTypeRef *parameters = NULL;
  if (signature->parameter_count != 0) {
    parameters = malloc(signature->parameter_count * sizeof(*parameters));
    if (parameters == NULL)
      return set_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while lowering function signature");
  }
  for (size_t i = 0; i < signature->parameter_count; ++i) {
    status = xs_llvm_primitive_type(unit->backend, signature->parameter_types[i], &parameters[i], error);
    if (status != XS_BACKEND_OK) {
      free(parameters);
      return status;
    }
    if (LLVMGetTypeKind(parameters[i]) == LLVMVoidTypeKind) {
      free(parameters);
      return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "function parameters cannot have unit type");
    }
  }
  LLVMTypeRef function_type = LLVMFunctionType(return_type, parameters, (unsigned)signature->parameter_count, false);
  free(parameters);
  *function = LLVMAddFunction(unit->module, signature->name, function_type);
  return XS_BACKEND_OK;
}

static XsBackendStatus verify_module(XsLlvmCodegenUnit *unit, XsBackendError *error)
{
  char *llvm_error = NULL;
  if (LLVMVerifyModule(unit->module, LLVMReturnStatusAction, &llvm_error) == 0)
    return XS_BACKEND_OK;
  XsBackendStatus status = set_error(error, XS_BACKEND_LLVM_ERROR, llvm_error);
  LLVMDisposeMessage(llvm_error);
  return status;
}

XsBackendStatus xs_llvm_optimize_codegen_unit(XsLlvmCodegenUnit *unit, XsBackendError *error)
{
  clear_error(error);
  if (unit == NULL)
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "codegen unit is required");
  if (unit->backend->verify_modules) {
    XsBackendStatus status = verify_module(unit, error);
    if (status != XS_BACKEND_OK)
      return status;
  }
  static const char *const pipelines[] = {"default<O0>", "default<O1>", "default<O2>", "default<O3>"};
  LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
  LLVMErrorRef llvm_error =
      LLVMRunPasses(unit->module, pipelines[unit->backend->optimization], unit->backend->target_machine, options);
  LLVMDisposePassBuilderOptions(options);
  if (llvm_error != NULL) {
    char *message = LLVMGetErrorMessage(llvm_error);
    XsBackendStatus status = set_error(error, XS_BACKEND_LLVM_ERROR, message);
    LLVMDisposeErrorMessage(message);
    return status;
  }
  return XS_BACKEND_OK;
}

XsBackendStatus xs_llvm_emit_object_file(XsLlvmCodegenUnit *unit, const char *path, XsBackendError *error)
{
  clear_error(error);
  if (unit == NULL || path == NULL || path[0] == '\0')
    return set_error(error, XS_BACKEND_INVALID_ARGUMENT, "codegen unit and object-file path are required");
  if (unit->backend->verify_modules) {
    XsBackendStatus status = verify_module(unit, error);
    if (status != XS_BACKEND_OK)
      return status;
  }
  char *mutable_path = copy_text(path);
  if (mutable_path == NULL)
    return set_error(error, XS_BACKEND_SYSTEM_ERROR, "out of memory while preparing object-file path");
  char *llvm_error = NULL;
  int failed = LLVMTargetMachineEmitToFile(unit->backend->target_machine, unit->module, mutable_path, LLVMObjectFile,
                                           &llvm_error);
  free(mutable_path);
  if (failed != 0) {
    XsBackendStatus status = set_error(error, XS_BACKEND_LLVM_ERROR, llvm_error);
    LLVMDisposeMessage(llvm_error);
    return status;
  }
  return XS_BACKEND_OK;
}
