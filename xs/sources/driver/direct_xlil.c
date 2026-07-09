/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "direct_xlil.h"

#include "xs/backend/llvm_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *direct_ir_output_path(const char *input_path)
{
  const char *slash = strrchr(input_path, '/');
  const char *base = slash == nullptr ? input_path : slash + 1;
  size_t base_length = strlen(base);
  if (base_length >= 5 && strcmp(base + base_length - 5, ".xlil") == 0)
    base_length -= 5;
  const char *extension = ".ll";
  size_t extension_length = strlen(extension);
  char *path = malloc(base_length + extension_length + 1U);
  if (path == nullptr)
    return nullptr;
  memcpy(path, base, base_length);
  memcpy(path + base_length, extension, extension_length + 1U);
  return path;
}

static bool declare_module_functions(XsLlvmCodegenUnit *unit, const XsLilModule *module, size_t *declared,
                                     XsBackendError *error)
{
  *declared = 0;
  size_t function_count = xs_lil_module_function_count(module);
  for (size_t i = 0; i < function_count; ++i)
  {
    const XsLilFunction *function = xs_lil_module_function_at(module, i);
    const char *name = xs_lil_function_name(function);
    size_t parameter_count = xs_lil_function_parameter_count(function);
    XsLilType *parameters = nullptr;
    if (parameter_count != 0)
    {
      parameters = malloc(parameter_count * sizeof(*parameters));
      if (parameters == nullptr)
      {
        error->status = XS_BACKEND_SYSTEM_ERROR;
        snprintf(error->message, sizeof(error->message), "%s",
                 "out of memory while preparing XLIL function declaration");
        return false;
      }
    }
    for (size_t parameter = 0; parameter < parameter_count; ++parameter)
      parameters[parameter] = xs_lil_function_parameter_type(function, parameter);
    LLVMValueRef llvm_function = nullptr;
    XsBackendStatus status = xs_llvm_declare_lil_function(unit, name, xs_lil_function_return_type(function), parameters,
                                                          parameter_count, &llvm_function, error);
    free(parameters);
    if (status != XS_BACKEND_OK)
      return false;
    ++*declared;
  }
  return true;
}

static bool lower_module_function_bodies(XsLlvmCodegenUnit *unit, const XsLilModule *module, size_t *lowered,
                                         XsBackendError *error)
{
  *lowered = 0;
  size_t function_count = xs_lil_module_function_count(module);
  for (size_t i = 0; i < function_count; ++i)
  {
    const XsLilFunction *function = xs_lil_module_function_at(module, i);
    if (!xs_lil_function_is_definition(function))
      continue;
    XsBackendStatus status = xs_llvm_lower_lil_function_body(unit, function, error);
    if (status != XS_BACKEND_OK)
      return false;
    ++*lowered;
  }
  return true;
}

bool xs_driver_emit_direct_xlil_llvm_ir(const char *input_path, const char *text, size_t length)
{
  XsLilError lil_error = {0};
  XsLilModule *module = nullptr;
  if (xs_lil_module_parse_text(input_path, text, length, &module, &lil_error) != XS_LIL_OK)
  {
    fprintf(stderr, "xs: XLIL parse failed: %s\n", lil_error.message);
    return false;
  }
  char *output_path = direct_ir_output_path(input_path);
  if (output_path == nullptr)
  {
    xs_lil_module_destroy(module);
    fprintf(stderr, "xs: out of memory while preparing LLVM IR output path\n");
    return false;
  }
  XsBackendError error = {0};
  XsLlvmBackend *backend = nullptr;
  XsLlvmCodegenUnit *unit = nullptr;
  XsLlvmBackendConfig config = {.optimization = XS_LLVM_OPT_NONE, .verify_modules = true};
  bool success = xs_llvm_backend_create(&config, &backend, &error) == XS_BACKEND_OK;
  if (success)
    success = xs_llvm_codegen_unit_create(backend, xs_lil_module_name(module), &unit, &error) == XS_BACKEND_OK;
  size_t declared = 0;
  if (success)
    success = declare_module_functions(unit, module, &declared, &error);
  size_t lowered = 0;
  if (success)
    success = lower_module_function_bodies(unit, module, &lowered, &error);
  if (success)
    success = xs_llvm_write_ir_file(unit, output_path, &error) == XS_BACKEND_OK;
  if (success)
    fprintf(stderr, "xs: wrote LLVM IR '%s' from XLIL module '%s' with %zu declaration(s), %zu body/bodies\n",
            output_path, xs_lil_module_name(module), declared, lowered);
  else
    fprintf(stderr, "xs: LLVM IR emission failed: %s\n", error.message);
  xs_llvm_codegen_unit_destroy(unit);
  xs_llvm_backend_destroy(backend);
  free(output_path);
  xs_lil_module_destroy(module);
  return success;
}
