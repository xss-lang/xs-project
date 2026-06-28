#include "xs/lil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  char *name;
  XsLilType return_type;
  XsLilType *parameters;
  size_t parameter_count;
} XsLilFunction;

struct XsLilModule
{
  char *name;
  XsLilFunction *functions;
  size_t function_count;
  size_t function_capacity;
};

static void clear_error(XsLilError *error)
{
  if (error != NULL)
    *error = (XsLilError){.status = XS_LIL_OK};
}

static XsLilStatus set_error(XsLilError *error, XsLilStatus status, const char *message)
{
  if (error != NULL) {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == NULL ? "XLIL error" : message);
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

static void function_free(XsLilFunction *function)
{
  free(function->name);
  free(function->parameters);
  *function = (XsLilFunction){0};
}

XsLilStatus xs_lil_module_create(const char *name, XsLilModule **module, XsLilError *error)
{
  clear_error(error);
  if (name == NULL || name[0] == '\0' || module == NULL)
    return set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL module name and output are required");
  *module = calloc(1, sizeof(**module));
  if (*module == NULL)
    return set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while creating XLIL module");
  (*module)->name = copy_text(name);
  if ((*module)->name == NULL) {
    xs_lil_module_destroy(*module);
    *module = NULL;
    return set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while naming XLIL module");
  }
  return XS_LIL_OK;
}

void xs_lil_module_destroy(XsLilModule *module)
{
  if (module == NULL)
    return;
  for (size_t i = 0; i < module->function_count; ++i)
    function_free(&module->functions[i]);
  free(module->functions);
  free(module->name);
  free(module);
}

const char *xs_lil_module_name(const XsLilModule *module)
{
  return module == NULL ? NULL : module->name;
}

static XsLilStatus append_function(XsLilModule *module, XsLilFunction function, XsLilError *error)
{
  if (module->function_count == module->function_capacity) {
    size_t capacity = module->function_capacity == 0 ? 8 : module->function_capacity * 2;
    XsLilFunction *functions = realloc(module->functions, capacity * sizeof(*functions));
    if (functions == NULL)
      return set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL function");
    module->functions = functions;
    module->function_capacity = capacity;
  }
  module->functions[module->function_count++] = function;
  return XS_LIL_OK;
}

XsLilStatus xs_lil_module_add_function(XsLilModule *module, const char *name, XsLilType return_type,
                                       const XsLilType *parameters, size_t parameter_count, XsLilError *error)
{
  clear_error(error);
  if (module == NULL || name == NULL || name[0] == '\0' || (parameter_count != 0 && parameters == NULL))
    return set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL module and function signature are required");
  XsLilFunction function = {
      .name = copy_text(name),
      .return_type = return_type,
      .parameter_count = parameter_count,
  };
  if (function.name == NULL) {
    function_free(&function);
    return set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while naming XLIL function");
  }
  if (parameter_count != 0) {
    function.parameters = malloc(parameter_count * sizeof(*function.parameters));
    if (function.parameters == NULL) {
      function_free(&function);
      return set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while copying XLIL parameters");
    }
    memcpy(function.parameters, parameters, parameter_count * sizeof(*function.parameters));
  }
  XsLilStatus status = append_function(module, function, error);
  if (status != XS_LIL_OK)
    function_free(&function);
  return status;
}

size_t xs_lil_module_function_count(const XsLilModule *module)
{
  return module == NULL ? 0 : module->function_count;
}

const char *xs_lil_type_name(XsLilType type)
{
  static const char *const names[] = {
      "void", "bool", "u8", "i8", "u16", "i16", "u32", "i32", "u64", "i64", "u128", "i128", "f16", "f32", "f64", "f128",
  };
  if ((size_t)type.kind >= sizeof(names) / sizeof(names[0]))
    return "unknown";
  return names[type.kind];
}

static XsLilStatus write_checked(FILE *stream, XsLilError *error, const char *text)
{
  if (fputs(text, stream) == EOF)
    return set_error(error, XS_LIL_IO_ERROR, "could not write XLIL text");
  return XS_LIL_OK;
}

XsLilStatus xs_lil_module_write_text(const XsLilModule *module, FILE *stream, XsLilError *error)
{
  clear_error(error);
  if (module == NULL || stream == NULL)
    return set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL module and stream are required");
  if (fprintf(stream, "xlil module %s\n", module->name) < 0)
    return set_error(error, XS_LIL_IO_ERROR, "could not write XLIL module header");
  for (size_t i = 0; i < module->function_count; ++i) {
    const XsLilFunction *function = &module->functions[i];
    if (fprintf(stream, "declare %s %s(", xs_lil_type_name(function->return_type), function->name) < 0)
      return set_error(error, XS_LIL_IO_ERROR, "could not write XLIL function declaration");
    for (size_t parameter = 0; parameter < function->parameter_count; ++parameter) {
      if (parameter != 0 && write_checked(stream, error, ", ") != XS_LIL_OK)
        return error == NULL ? XS_LIL_IO_ERROR : error->status;
      if (write_checked(stream, error, xs_lil_type_name(function->parameters[parameter])) != XS_LIL_OK)
        return error == NULL ? XS_LIL_IO_ERROR : error->status;
    }
    if (write_checked(stream, error, ")\n") != XS_LIL_OK)
      return error == NULL ? XS_LIL_IO_ERROR : error->status;
  }
  return XS_LIL_OK;
}
