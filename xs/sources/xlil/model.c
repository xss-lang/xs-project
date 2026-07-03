#include "model_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void xs_lil_clear_error(XsLilError *error)
{
  if (error != NULL)
    *error = (XsLilError){.status = XS_LIL_OK};
}

XsLilStatus xs_lil_set_error(XsLilError *error, XsLilStatus status, const char *message)
{
  if (error != NULL)
  {
    error->status = status;
    snprintf(error->message, sizeof(error->message), "%s", message == NULL ? "XLIL error" : message);
  }
  return status;
}

char *xs_lil_copy_text(const char *text)
{
  size_t length = strlen(text);
  char *copy = malloc(length + 1);
  if (copy != NULL)
    memcpy(copy, text, length + 1);
  return copy;
}

XsLilStatus xs_lil_write_checked(FILE *stream, XsLilError *error, const char *text)
{
  if (fputs(text, stream) == EOF)
    return xs_lil_set_error(error, XS_LIL_IO_ERROR, "could not write XLIL text");
  return XS_LIL_OK;
}

static void function_free(XsLilFunction *function)
{
  for (size_t i = 0; i < function->block_count; ++i)
  {
    XsLilBlock *block = function->blocks[i];
    free(block->label);
    free(block->instructions);
    free(block);
  }
  free(function->blocks);
  free(function->values);
  free(function->name);
  free(function->parameters);
  *function = (XsLilFunction){0};
}

XsLilStatus xs_lil_module_create(const char *name, XsLilModule **module, XsLilError *error)
{
  xs_lil_clear_error(error);
  if (name == NULL || name[0] == '\0' || module == NULL)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL module name and output are required");
  *module = calloc(1, sizeof(**module));
  if (*module == NULL)
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while creating XLIL module");
  (*module)->name = xs_lil_copy_text(name);
  if ((*module)->name == NULL)
  {
    xs_lil_module_destroy(*module);
    *module = NULL;
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while naming XLIL module");
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
  if (module->function_count == module->function_capacity)
  {
    size_t capacity = module->function_capacity == 0 ? 8 : module->function_capacity * 2;
    XsLilFunction *functions = realloc(module->functions, capacity * sizeof(*functions));
    if (functions == NULL)
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL function");
    module->functions = functions;
    module->function_capacity = capacity;
  }
  module->functions[module->function_count++] = function;
  return XS_LIL_OK;
}

static XsLilStatus add_function(XsLilModule *module, const char *name, XsLilType return_type,
                                const XsLilType *parameters, size_t parameter_count, bool is_definition,
                                XsLilFunction **out_function, XsLilError *error)
{
  xs_lil_clear_error(error);
  if (out_function != NULL)
    *out_function = NULL;
  if (module == NULL || name == NULL || name[0] == '\0' || (parameter_count != 0 && parameters == NULL))
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL module and function signature are required");
  XsLilFunction function = {
      .name = xs_lil_copy_text(name),
      .return_type = return_type,
      .parameter_count = parameter_count,
      .is_definition = is_definition,
  };
  if (function.name == NULL)
  {
    function_free(&function);
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while naming XLIL function");
  }
  if (parameter_count != 0)
  {
    function.parameters = malloc(parameter_count * sizeof(*function.parameters));
    if (function.parameters == NULL)
    {
      function_free(&function);
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while copying XLIL parameters");
    }
    memcpy(function.parameters, parameters, parameter_count * sizeof(*function.parameters));
  }
  XsLilStatus status = append_function(module, function, error);
  if (status != XS_LIL_OK)
    function_free(&function);
  else if (out_function != NULL)
    *out_function = &module->functions[module->function_count - 1];
  return status;
}

XsLilStatus xs_lil_module_add_function(XsLilModule *module, const char *name, XsLilType return_type,
                                       const XsLilType *parameters, size_t parameter_count, XsLilError *error)
{
  return add_function(module, name, return_type, parameters, parameter_count, false, NULL, error);
}

XsLilStatus xs_lil_module_add_function_definition(XsLilModule *module, const char *name, XsLilType return_type,
                                                  const XsLilType *parameters, size_t parameter_count,
                                                  XsLilFunction **function, XsLilError *error)
{
  return add_function(module, name, return_type, parameters, parameter_count, true, function, error);
}

size_t xs_lil_module_function_count(const XsLilModule *module)
{
  return module == NULL ? 0 : module->function_count;
}

static XsLilStatus add_value(XsLilFunction *function, XsLilType type, XsLilValueId *value, XsLilError *error)
{
  if (function->value_count == function->value_capacity)
  {
    size_t capacity = function->value_capacity == 0 ? 8 : function->value_capacity * 2;
    XsLilValue *values = realloc(function->values, capacity * sizeof(*values));
    if (values == NULL)
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL value");
    function->values = values;
    function->value_capacity = capacity;
  }
  *value = (XsLilValueId)function->value_count;
  function->values[function->value_count++] = (XsLilValue){.type = type};
  return XS_LIL_OK;
}

XsLilStatus xs_lil_function_append_block(XsLilFunction *function, const char *label, XsLilBlock **block,
                                         XsLilError *error)
{
  xs_lil_clear_error(error);
  if (block != NULL)
    *block = NULL;
  if (function == NULL || !function->is_definition || label == NULL || label[0] == '\0')
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT,
                            "valid XLIL function definition and block label are required");
  if (function->block_count == function->block_capacity)
  {
    size_t capacity = function->block_capacity == 0 ? 4 : function->block_capacity * 2;
    XsLilBlock **blocks = realloc(function->blocks, capacity * sizeof(*blocks));
    if (blocks == NULL)
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL block");
    function->blocks = blocks;
    function->block_capacity = capacity;
  }
  XsLilBlock *created = calloc(1, sizeof(*created));
  if (created == NULL)
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL block");
  *created = (XsLilBlock){.label = xs_lil_copy_text(label), .owner = function, .id = (uint32_t)function->block_count};
  if (created->label == NULL)
  {
    free(created);
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while naming XLIL block");
  }
  function->blocks[function->block_count++] = created;
  if (block != NULL)
    *block = created;
  return XS_LIL_OK;
}

static XsLilStatus append_instruction(XsLilBlock *block, XsLilInstruction instruction, XsLilError *error)
{
  if (block == NULL)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL block is required");
  if (block->terminator.kind != XS_LIL_TERMINATOR_NONE)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "cannot add XLIL instruction after terminator");
  if (block->instruction_count == block->instruction_capacity)
  {
    size_t capacity = block->instruction_capacity == 0 ? 8 : block->instruction_capacity * 2;
    XsLilInstruction *instructions = realloc(block->instructions, capacity * sizeof(*instructions));
    if (instructions == NULL)
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL instruction");
    block->instructions = instructions;
    block->instruction_capacity = capacity;
  }
  block->instructions[block->instruction_count++] = instruction;
  return XS_LIL_OK;
}

XsLilStatus xs_lil_block_add_const_i64(XsLilBlock *block, int64_t value, XsLilValueId *result, XsLilError *error)
{
  xs_lil_clear_error(error);
  if (result != NULL)
    *result = 0;
  if (block == NULL || block->owner == NULL)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL block is required");
  XsLilValueId value_id = 0;
  XsLilStatus status = add_value(block->owner, (XsLilType){.kind = XS_LIL_TYPE_I64}, &value_id, error);
  if (status != XS_LIL_OK)
    return status;
  status = append_instruction(block,
                              (XsLilInstruction){
                                  .kind = XS_LIL_INSTRUCTION_CONST_I64,
                                  .result = value_id,
                                  .immediate_i64 = value,
                              },
                              error);
  if (status != XS_LIL_OK)
    return status;
  if (result != NULL)
    *result = value_id;
  return XS_LIL_OK;
}

static XsLilStatus set_terminator(XsLilBlock *block, XsLilTerminator terminator, XsLilError *error)
{
  xs_lil_clear_error(error);
  if (block == NULL)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "valid XLIL block is required");
  if (block->terminator.kind != XS_LIL_TERMINATOR_NONE)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL block already has a terminator");
  if (terminator.kind == XS_LIL_TERMINATOR_RETURN && terminator.has_value)
  {
    if ((size_t)terminator.value >= block->owner->value_count)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL return references an unknown value");
    if (block->owner->values[terminator.value].type.kind != block->owner->return_type.kind)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT,
                              "XLIL return value type does not match function return type");
  }
  else if (terminator.kind == XS_LIL_TERMINATOR_RETURN && block->owner->return_type.kind != XS_LIL_TYPE_VOID)
  {
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL return is missing a value");
  }
  block->terminator = terminator;
  return XS_LIL_OK;
}

XsLilStatus xs_lil_block_set_return(XsLilBlock *block, XsLilError *error)
{
  return set_terminator(block, (XsLilTerminator){.kind = XS_LIL_TERMINATOR_RETURN}, error);
}

XsLilStatus xs_lil_block_set_return_value(XsLilBlock *block, XsLilValueId value, XsLilError *error)
{
  return set_terminator(block, (XsLilTerminator){.kind = XS_LIL_TERMINATOR_RETURN, .has_value = true, .value = value},
                        error);
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
