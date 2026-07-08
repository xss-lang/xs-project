/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "direct_xlil.h"

#include "xs/backend/llvm_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  char *name;
  XsLilType return_type;
  XsLilType *parameters;
  size_t parameter_count;
} DirectXlilSignature;

static void direct_xlil_signature_free(DirectXlilSignature *signature)
{
  free(signature->parameters);
  free(signature->name);
  *signature = (DirectXlilSignature){0};
}

static const char *skip_inline_space(const char *cursor, const char *end)
{
  while (cursor < end && (*cursor == ' ' || *cursor == '\t'))
    ++cursor;
  return cursor;
}

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

static char *direct_xlil_module_name(const char *text, size_t length)
{
  size_t cursor = 0;
  while (cursor < length && text[cursor] != '\n')
    ++cursor;
  if (cursor < length && text[cursor] == '\n')
    ++cursor;
  size_t line_start = cursor;
  while (cursor < length && text[cursor] != '\n' && text[cursor] != '\r')
    ++cursor;
  const char *prefix = ".xlil module ";
  size_t prefix_length = strlen(prefix);
  if (cursor - line_start <= prefix_length || strncmp(text + line_start, prefix, prefix_length) != 0)
    return nullptr;
  size_t name_length = cursor - line_start - prefix_length;
  char *name = malloc(name_length + 1U);
  if (name == nullptr)
    return nullptr;
  memcpy(name, text + line_start + prefix_length, name_length);
  name[name_length] = '\0';
  return name;
}

static bool xlil_type_from_token(const char *start, size_t length, XsLilType *type)
{
  static const struct
  {
    const char *name;
    XsLilTypeKind kind;
  } types[] = {
      {"void", XS_LIL_TYPE_VOID}, {"bool", XS_LIL_TYPE_BOOL}, {"u8", XS_LIL_TYPE_U8},     {"i8", XS_LIL_TYPE_I8},
      {"u16", XS_LIL_TYPE_U16},   {"i16", XS_LIL_TYPE_I16},   {"u32", XS_LIL_TYPE_U32},   {"i32", XS_LIL_TYPE_I32},
      {"u64", XS_LIL_TYPE_U64},   {"i64", XS_LIL_TYPE_I64},   {"u128", XS_LIL_TYPE_U128}, {"i128", XS_LIL_TYPE_I128},
      {"f16", XS_LIL_TYPE_F16},   {"f32", XS_LIL_TYPE_F32},   {"f64", XS_LIL_TYPE_F64},   {"f128", XS_LIL_TYPE_F128},
  };
  for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
  {
    if (strlen(types[i].name) == length && strncmp(start, types[i].name, length) == 0)
    {
      *type = (XsLilType){.kind = types[i].kind};
      return true;
    }
  }
  return false;
}

static bool append_xlil_parameter(DirectXlilSignature *signature, XsLilType type)
{
  XsLilType *grown = realloc(signature->parameters, (signature->parameter_count + 1U) * sizeof(*grown));
  if (grown == nullptr)
    return false;
  signature->parameters = grown;
  signature->parameters[signature->parameter_count++] = type;
  return true;
}

static bool parse_xlil_type_cursor(const char **cursor, const char *end, XsLilType *type)
{
  const char *start = skip_inline_space(*cursor, end);
  const char *type_end = start;
  while (type_end < end && *type_end != ' ' && *type_end != '\t' && *type_end != ',' && *type_end != ')')
    ++type_end;
  if (type_end == start || !xlil_type_from_token(start, (size_t)(type_end - start), type))
    return false;
  *cursor = skip_inline_space(type_end, end);
  return true;
}

static bool parse_direct_xlil_signature(const char *line, size_t length, DirectXlilSignature *signature)
{
  const char *end = line + length;
  const char *cursor = skip_inline_space(line, end);
  const char *prefix = nullptr;
  if ((size_t)(end - cursor) >= 8U && strncmp(cursor, ".extern ", 8U) == 0)
    prefix = ".extern ";
  else if ((size_t)(end - cursor) >= 6U && strncmp(cursor, ".func ", 6U) == 0)
    prefix = ".func ";
  else
    return false;
  cursor += strlen(prefix);
  const char *colon = cursor;
  while (colon < end && *colon != ':')
    ++colon;
  if (colon == end)
    return false;
  const char *name_end = colon;
  while (name_end > cursor && (name_end[-1] == ' ' || name_end[-1] == '\t'))
    --name_end;
  if (name_end == cursor)
    return false;
  signature->name = malloc((size_t)(name_end - cursor) + 1U);
  if (signature->name == nullptr)
    return false;
  memcpy(signature->name, cursor, (size_t)(name_end - cursor));
  signature->name[name_end - cursor] = '\0';

  cursor = skip_inline_space(colon + 1, end);
  if (cursor == end || *cursor++ != '(')
    return false;
  cursor = skip_inline_space(cursor, end);
  while (cursor < end && *cursor != ')')
  {
    XsLilType parameter = {0};
    if (!parse_xlil_type_cursor(&cursor, end, &parameter) || parameter.kind == XS_LIL_TYPE_VOID)
      return false;
    if (!append_xlil_parameter(signature, parameter))
      return false;
    if (cursor < end && *cursor == ',')
    {
      cursor = skip_inline_space(cursor + 1, end);
      if (cursor < end && *cursor == ')')
        return false;
    }
    else if (cursor < end && *cursor != ')')
    {
      return false;
    }
  }
  if (cursor == end || *cursor++ != ')')
    return false;
  cursor = skip_inline_space(cursor, end);
  if ((size_t)(end - cursor) < 2U || cursor[0] != '-' || cursor[1] != '>')
    return false;
  cursor = skip_inline_space(cursor + 2, end);
  if (!parse_xlil_type_cursor(&cursor, end, &signature->return_type))
    return false;
  return skip_inline_space(cursor, end) == end;
}

static bool declare_direct_xlil_signatures(XsLlvmCodegenUnit *unit, const char *path, const char *text, size_t length,
                                           size_t *declared, XsBackendError *error)
{
  size_t cursor = 0;
  size_t line_number = 1;
  *declared = 0;
  while (cursor < length)
  {
    size_t line_start = cursor;
    while (cursor < length && text[cursor] != '\n' && text[cursor] != '\r')
      ++cursor;
    size_t line_length = cursor - line_start;
    const char *line = text + line_start;
    const char *trimmed = skip_inline_space(line, line + line_length);
    size_t trimmed_length = (size_t)(line + line_length - trimmed);
    bool is_signature = (trimmed_length >= 6U && strncmp(trimmed, ".func ", 6U) == 0) ||
                        (trimmed_length >= 8U && strncmp(trimmed, ".extern ", 8U) == 0);
    if (is_signature)
    {
      DirectXlilSignature signature = {0};
      if (!parse_direct_xlil_signature(line, line_length, &signature))
      {
        direct_xlil_signature_free(&signature);
        fprintf(stderr, "xs: invalid XLIL function signature in '%s' at line %zu\n", path, line_number);
        return false;
      }
      LLVMValueRef function = nullptr;
      XsBackendStatus status =
          xs_llvm_declare_lil_function(unit, signature.name, signature.return_type, signature.parameters,
                                       signature.parameter_count, &function, error);
      direct_xlil_signature_free(&signature);
      if (status != XS_BACKEND_OK)
        return false;
      ++*declared;
    }
    if (cursor < length && text[cursor] == '\r')
      ++cursor;
    if (cursor < length && text[cursor] == '\n')
      ++cursor;
    ++line_number;
  }
  return true;
}

bool xs_driver_emit_direct_xlil_llvm_ir(const char *input_path, const char *text, size_t length)
{
  char *module_name = direct_xlil_module_name(text, length);
  if (module_name == nullptr)
  {
    fprintf(stderr, "xs: XLIL file '%s' has an invalid module header\n", input_path);
    return false;
  }
  char *output_path = direct_ir_output_path(input_path);
  if (output_path == nullptr)
  {
    free(module_name);
    fprintf(stderr, "xs: out of memory while preparing LLVM IR output path\n");
    return false;
  }
  XsBackendError error = {0};
  XsLlvmBackend *backend = nullptr;
  XsLlvmCodegenUnit *unit = nullptr;
  XsLlvmBackendConfig config = {.optimization = XS_LLVM_OPT_NONE, .verify_modules = true};
  bool success = xs_llvm_backend_create(&config, &backend, &error) == XS_BACKEND_OK;
  if (success)
    success = xs_llvm_codegen_unit_create(backend, module_name, &unit, &error) == XS_BACKEND_OK;
  size_t declared = 0;
  if (success)
    success = declare_direct_xlil_signatures(unit, input_path, text, length, &declared, &error);
  if (success)
    success = xs_llvm_write_ir_file(unit, output_path, &error) == XS_BACKEND_OK;
  if (success)
    fprintf(stderr, "xs: wrote LLVM IR '%s' from XLIL module '%s' with %zu declaration(s)\n", output_path, module_name,
            declared);
  else
    fprintf(stderr, "xs: LLVM IR emission failed: %s\n", error.message);
  xs_llvm_codegen_unit_destroy(unit);
  xs_llvm_backend_destroy(backend);
  free(output_path);
  free(module_name);
  return success;
}
