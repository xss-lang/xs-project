#include "model_internal.h"

#include <stdio.h>

static XsMirStatus write_signature(const XsMirFunction *function, FILE *stream, XsMirError *error)
{
  if (fprintf(stream, "%s fn %s(", function->is_definition ? "define" : "declare", function->qualified_name) < 0)
    return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR function signature");
  for (size_t parameter = 0; parameter < function->parameter_count; ++parameter) {
    if (parameter != 0 && fputs(", ", stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR function parameter separator");
    if (fputs(xs_lil_type_name(function->parameters[parameter]), stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR function parameter type");
  }
  if (fprintf(stream, ") -> %s", xs_lil_type_name(function->return_type)) < 0)
    return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR function return type");
  return XS_MIR_OK;
}

static XsMirStatus write_terminator(const XsMirBlock *block, FILE *stream, XsMirError *error)
{
  switch (block->terminator.kind) {
  case XS_MIR_TERMINATOR_RETURN:
    if (block->terminator.has_value) {
      if (fprintf(stream, "  return %u\n", block->terminator.value) < 0)
        return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR return terminator");
      return XS_MIR_OK;
    }
    if (fputs("  return\n", stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR return terminator");
    return XS_MIR_OK;
  case XS_MIR_TERMINATOR_GOTO:
    if (fprintf(stream, "  goto bb%u\n", block->terminator.target) < 0)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR goto terminator");
    return XS_MIR_OK;
  case XS_MIR_TERMINATOR_UNREACHABLE:
    if (fputs("  unreachable\n", stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR unreachable terminator");
    return XS_MIR_OK;
  case XS_MIR_TERMINATOR_NONE:
    if (fputs("  <missing terminator>\n", stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR missing terminator marker");
    return XS_MIR_OK;
  }
  return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "unknown MIR terminator kind");
}

static const char *local_kind_name(XsMirLocalKind kind)
{
  switch (kind) {
  case XS_MIR_LOCAL_PARAMETER:
    return "param";
  case XS_MIR_LOCAL_VARIABLE:
    return "var";
  case XS_MIR_LOCAL_TEMPORARY:
    return "temp";
  }
  return "local";
}

static XsMirStatus write_instruction(const XsMirInstruction *instruction, FILE *stream, XsMirError *error)
{
  switch (instruction->kind) {
  case XS_MIR_INSTRUCTION_CONST_I64:
    if (fprintf(stream, "  v%u = const.i64 %lld\n", instruction->result, (long long)instruction->immediate_i64) < 0)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR const.i64 instruction");
    return XS_MIR_OK;
  case XS_MIR_INSTRUCTION_LOAD:
    if (fprintf(stream, "  v%u = load place%u\n", instruction->result, instruction->place) < 0)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR load instruction");
    return XS_MIR_OK;
  case XS_MIR_INSTRUCTION_STORE:
    if (fprintf(stream, "  store place%u, v%u\n", instruction->place, instruction->operand) < 0)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR store instruction");
    return XS_MIR_OK;
  }
  return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "unknown MIR instruction kind");
}

XsMirStatus xs_mir_module_write_text(const XsMirModule *module, FILE *stream, XsMirError *error)
{
  xs_mir_clear_error(error);
  if (module == NULL || stream == NULL)
    return xs_mir_set_error(error, XS_MIR_INVALID_ARGUMENT, "MIR module and stream are required");
  if (fprintf(stream, "mir module %s\n", module->name) < 0)
    return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR module header");
  for (size_t i = 0; i < module->function_count; ++i) {
    const XsMirFunction *function = &module->functions[i];
    XsMirStatus status = write_signature(function, stream, error);
    if (status != XS_MIR_OK)
      return status;
    if (!function->is_definition) {
      if (fputc('\n', stream) == EOF)
        return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not finish MIR declaration");
      continue;
    }
    if (fputs(" {\n", stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not open MIR function body");
    for (size_t local = 0; local < function->local_count; ++local) {
      const XsMirLocal *entry = &function->locals[local];
      if (fprintf(stream, "local %zu %s %s%s: %s\n", local, local_kind_name(entry->kind),
                  entry->is_mutable ? "mut " : "", entry->name, xs_lil_type_name(entry->type)) < 0)
        return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR local");
    }
    for (size_t block_index = 0; block_index < function->block_count; ++block_index) {
      const XsMirBlock *block = function->blocks[block_index];
      if (fprintf(stream, "bb%u %s:\n", block->id, block->label) < 0)
        return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not write MIR block label");
      for (size_t instruction = 0; instruction < block->instruction_count; ++instruction) {
        status = write_instruction(&block->instructions[instruction], stream, error);
        if (status != XS_MIR_OK)
          return status;
      }
      status = write_terminator(block, stream, error);
      if (status != XS_MIR_OK)
        return status;
    }
    if (fputs("}\n", stream) == EOF)
      return xs_mir_set_error(error, XS_MIR_IO_ERROR, "could not close MIR function body");
  }
  return XS_MIR_OK;
}
