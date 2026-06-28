#include "xs/mir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct XsMirFunction
{
  char *qualified_name;
  XsMirType return_type;
  XsMirType *parameters;
  size_t parameter_count;
};

struct XsMirModule
{
  char *name;
  XsMirFunction *functions;
  size_t function_count;
  size_t function_capacity;
};

static void clear_error(XsMirError *error)
{
  if (error != NULL)
    *error = (XsMirError){.status = XS_MIR_OK};
}

static XsMirStatus set_error(XsMirError *error, XsMirStatus status, const char *message)
{
  if (error != NULL) {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == NULL ? "MIR error" : message);
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

static void function_free(XsMirFunction *function)
{
  free(function->qualified_name);
  free(function->parameters);
  *function = (XsMirFunction){0};
}

XsMirStatus xs_mir_module_create(const char *name, XsMirModule **module, XsMirError *error)
{
  clear_error(error);
  if (name == NULL || name[0] == '\0' || module == NULL)
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR module name and output are required");
  *module = calloc(1, sizeof(**module));
  if (*module == NULL)
    return set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while creating MIR module");
  (*module)->name = copy_text(name);
  if ((*module)->name == NULL) {
    xs_mir_module_destroy(*module);
    *module = NULL;
    return set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while naming MIR module");
  }
  return XS_MIR_OK;
}

void xs_mir_module_destroy(XsMirModule *module)
{
  if (module == NULL)
    return;
  for (size_t i = 0; i < module->function_count; ++i)
    function_free(&module->functions[i]);
  free(module->functions);
  free(module->name);
  free(module);
}

const char *xs_mir_module_name(const XsMirModule *module)
{
  return module == NULL ? NULL : module->name;
}

static XsMirStatus append_function(XsMirModule *module, XsMirFunction function, XsMirError *error)
{
  if (module->function_count == module->function_capacity) {
    size_t capacity = module->function_capacity == 0 ? 8 : module->function_capacity * 2;
    XsMirFunction *functions = realloc(module->functions, capacity * sizeof(*functions));
    if (functions == NULL)
      return set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while adding MIR function");
    module->functions = functions;
    module->function_capacity = capacity;
  }
  module->functions[module->function_count++] = function;
  return XS_MIR_OK;
}

XsMirStatus xs_mir_module_add_function_declaration(XsMirModule *module, const char *qualified_name,
                                                   XsMirType return_type, const XsMirType *parameters,
                                                   size_t parameter_count, XsMirError *error)
{
  clear_error(error);
  if (module == NULL || qualified_name == NULL || qualified_name[0] == '\0' ||
      (parameter_count != 0 && parameters == NULL))
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "valid MIR module and function signature are required");
  XsMirFunction function = {
      .qualified_name = copy_text(qualified_name),
      .return_type = return_type,
      .parameter_count = parameter_count,
  };
  if (function.qualified_name == NULL)
    return set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while naming MIR function");
  if (parameter_count != 0) {
    function.parameters = malloc(parameter_count * sizeof(*function.parameters));
    if (function.parameters == NULL) {
      function_free(&function);
      return set_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while copying MIR parameters");
    }
    memcpy(function.parameters, parameters, parameter_count * sizeof(*function.parameters));
  }
  XsMirStatus status = append_function(module, function, error);
  if (status != XS_MIR_OK)
    function_free(&function);
  return status;
}

size_t xs_mir_module_function_count(const XsMirModule *module)
{
  return module == NULL ? 0 : module->function_count;
}

const XsMirFunction *xs_mir_module_function_at(const XsMirModule *module, size_t index)
{
  if (module == NULL || index >= module->function_count)
    return NULL;
  return &module->functions[index];
}

const char *xs_mir_function_name(const XsMirFunction *function)
{
  return function == NULL ? NULL : function->qualified_name;
}

XsMirType xs_mir_function_return_type(const XsMirFunction *function)
{
  return function == NULL ? (XsMirType){.kind = XS_LIL_TYPE_VOID} : function->return_type;
}

const XsMirType *xs_mir_function_parameters(const XsMirFunction *function)
{
  return function == NULL ? NULL : function->parameters;
}

size_t xs_mir_function_parameter_count(const XsMirFunction *function)
{
  return function == NULL ? 0 : function->parameter_count;
}

XsMirStatus xs_mir_module_write_text(const XsMirModule *module, FILE *stream, XsMirError *error)
{
  clear_error(error);
  if (module == NULL || stream == NULL)
    return set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR module and stream are required");
  if (fprintf(stream, "mir module %s\n", module->name) < 0)
    return set_error(error, XS_MIR_IO_ERROR, "could not write MIR module header");
  for (size_t i = 0; i < module->function_count; ++i) {
    const XsMirFunction *function = &module->functions[i];
    if (fprintf(stream, "declare fn %s(", function->qualified_name) < 0)
      return set_error(error, XS_MIR_IO_ERROR, "could not write MIR function declaration");
    for (size_t parameter = 0; parameter < function->parameter_count; ++parameter) {
      if (parameter != 0 && fputs(", ", stream) == EOF)
        return set_error(error, XS_MIR_IO_ERROR, "could not write MIR function parameter separator");
      if (fputs(xs_lil_type_name(function->parameters[parameter]), stream) == EOF)
        return set_error(error, XS_MIR_IO_ERROR, "could not write MIR function parameter type");
    }
    if (fprintf(stream, ") -> %s\n", xs_lil_type_name(function->return_type)) < 0)
      return set_error(error, XS_MIR_IO_ERROR, "could not write MIR function return type");
  }
  return XS_MIR_OK;
}
