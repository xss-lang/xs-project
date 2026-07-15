/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model_internal.h"

#include <stdlib.h>
#include <string.h>

XsLilType xs_lil_aggregate_type(uint32_t registry_id)
{
  return (XsLilType){.kind = XS_LIL_TYPE_AGGREGATE, .registry_id = registry_id};
}

bool xs_lil_type_equal(XsLilType left, XsLilType right)
{
  return left.kind == right.kind && (left.kind != XS_LIL_TYPE_AGGREGATE || left.registry_id == right.registry_id);
}

static bool valid_field(const XsLilModule *module, XsLilType field)
{
  if(field.kind == XS_LIL_TYPE_VOID || (unsigned)field.kind > (unsigned)XS_LIL_TYPE_AGGREGATE)
    return false;
  return field.kind != XS_LIL_TYPE_AGGREGATE || field.registry_id < module->aggregate_type_count;
}

XsLilStatus xs_lil_module_add_aggregate_type(XsLilModule *module, const char *name, const XsLilType *fields,
                                             size_t field_count, XsLilType *type, XsLilError *error)
{
  xs_lil_clear_error(error);
  if(module == nullptr || name == nullptr || name[0] == '\0' || type == nullptr ||
     (field_count != 0 && fields == nullptr))
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL aggregate name, fields, and output are required");
  for(size_t i = 0; i < module->aggregate_type_count; ++i)
    if(strcmp(module->aggregate_types[i].name, name) == 0)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL aggregate type name is duplicated");
  for(size_t i = 0; i < field_count; ++i)
    if(!valid_field(module, fields[i]))
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT,
                              "XLIL aggregate fields must reference known non-void types");
  if(module->aggregate_type_count == module->aggregate_type_capacity)
  {
    size_t capacity = module->aggregate_type_capacity == 0 ? 4 : module->aggregate_type_capacity * 2;
    XsLilAggregateType *entries = realloc(module->aggregate_types, capacity * sizeof(*entries));
    if(entries == nullptr)
      return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL aggregate type");
    module->aggregate_types = entries;
    module->aggregate_type_capacity = capacity;
  }
  char *name_copy = xs_lil_copy_text(name);
  XsLilType *field_copy = field_count == 0 ? nullptr : malloc(field_count * sizeof(*field_copy));
  if(name_copy == nullptr || (field_count != 0 && field_copy == nullptr))
  {
    free(name_copy);
    free(field_copy);
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while copying XLIL aggregate type");
  }
  if(field_count != 0)
    memcpy(field_copy, fields, field_count * sizeof(*field_copy));
  uint32_t id = (uint32_t)module->aggregate_type_count;
  module->aggregate_types[module->aggregate_type_count++] =
      (XsLilAggregateType){.name = name_copy, .fields = field_copy, .field_count = field_count};
  *type = xs_lil_aggregate_type(id);
  return XS_LIL_OK;
}

size_t xs_lil_module_aggregate_type_count(const XsLilModule *module)
{
  return module == nullptr ? 0 : module->aggregate_type_count;
}

const char *xs_lil_module_aggregate_type_name(const XsLilModule *module, uint32_t registry_id)
{
  return module == nullptr || registry_id >= module->aggregate_type_count ? nullptr
                                                                          : module->aggregate_types[registry_id].name;
}

size_t xs_lil_module_aggregate_field_count(const XsLilModule *module, uint32_t registry_id)
{
  return module == nullptr || registry_id >= module->aggregate_type_count
             ? 0
             : module->aggregate_types[registry_id].field_count;
}

XsLilType xs_lil_module_aggregate_field_type(const XsLilModule *module, uint32_t registry_id, size_t field)
{
  if(module == nullptr || registry_id >= module->aggregate_type_count ||
     field >= module->aggregate_types[registry_id].field_count)
    return (XsLilType){.kind = XS_LIL_TYPE_VOID};
  return module->aggregate_types[registry_id].fields[field];
}

XsLilStatus xs_lil_block_add_aggregate(XsLilBlock *block, XsLilType type, const XsLilValueId *fields,
                                       size_t field_count, XsLilValueId *result, XsLilError *error)
{
  if(block == nullptr || block->owner == nullptr || type.kind != XS_LIL_TYPE_AGGREGATE || result == nullptr ||
     (field_count != 0 && fields == nullptr))
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL aggregate instruction arguments are invalid");
  for(size_t i = 0; i < field_count; ++i)
    if(fields[i] >= block->owner->value_count)
      return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL aggregate field value is unknown");
  XsLilValueId value = UINT32_MAX;
  XsLilStatus status = xs_lil_add_value(block->owner, type, &value, error);
  if(status != XS_LIL_OK)
    return status;
  XsLilValueId *copy = field_count == 0 ? nullptr : malloc(field_count * sizeof(*copy));
  if(field_count != 0 && copy == nullptr)
    return xs_lil_set_error(error, XS_LIL_ALLOCATION_FAILED, "out of memory while adding XLIL aggregate fields");
  if(field_count != 0)
    memcpy(copy, fields, field_count * sizeof(*copy));
  XsLilInstruction instruction = {.kind = XS_LIL_INSTRUCTION_AGGREGATE,
                                  .result = value,
                                  .integer_type = type,
                                  .arguments = copy,
                                  .argument_count = field_count};
  status = xs_lil_append_instruction(block, instruction, error);
  if(status != XS_LIL_OK)
  {
    free(copy);
    return status;
  }
  *result = value;
  return XS_LIL_OK;
}

XsLilStatus xs_lil_block_add_extract(XsLilBlock *block, XsLilValueId aggregate, uint32_t field, XsLilType field_type,
                                     XsLilValueId *result, XsLilError *error)
{
  if(block == nullptr || block->owner == nullptr || aggregate >= block->owner->value_count || result == nullptr ||
     block->owner->values[aggregate].type.kind != XS_LIL_TYPE_AGGREGATE || field_type.kind == XS_LIL_TYPE_VOID)
    return xs_lil_set_error(error, XS_LIL_INVALID_ARGUMENT, "XLIL extract instruction arguments are invalid");
  XsLilValueId value = UINT32_MAX;
  XsLilStatus status = xs_lil_add_value(block->owner, field_type, &value, error);
  if(status != XS_LIL_OK)
    return status;
  status = xs_lil_append_instruction(
      block,
      (XsLilInstruction){
          .kind = XS_LIL_INSTRUCTION_EXTRACT, .result = value, .left = aggregate, .immediate_i64 = field},
      error);
  if(status == XS_LIL_OK)
    *result = value;
  return status;
}
