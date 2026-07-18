/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_LIL_C_FUNCTION_H
#define XS_LIL_C_FUNCTION_H

#include "xs/lil-c/module.h"

XsLilStatus xs_lil_module_add_function(XsLilModule *module, const char *name, XsLilType return_type,
                                       const XsLilType *parameters, size_t parameter_count, XsLilError *error);
XsLilStatus xs_lil_module_add_function_definition(XsLilModule *module, const char *name, XsLilType return_type,
                                                  const XsLilType *parameters, size_t parameter_count,
                                                  XsLilFunction **function, XsLilError *error);
size_t xs_lil_module_function_count(const XsLilModule *module);
const XsLilFunction *xs_lil_module_function_at(const XsLilModule *module, size_t index);
const char *xs_lil_function_name(const XsLilFunction *function);
XsLilType xs_lil_function_return_type(const XsLilFunction *function);
size_t xs_lil_function_parameter_count(const XsLilFunction *function);
XsLilType xs_lil_function_parameter_type(const XsLilFunction *function, size_t index);
bool xs_lil_function_is_definition(const XsLilFunction *function);
size_t xs_lil_function_value_count(const XsLilFunction *function);
XsLilType xs_lil_function_value_type(const XsLilFunction *function, XsLilValueId value);
XsLilStatus xs_lil_function_add_slot(XsLilFunction *function, XsLilType type, XsLilSlotId *slot, XsLilError *error);
size_t xs_lil_function_slot_count(const XsLilFunction *function);
XsLilType xs_lil_function_slot_type(const XsLilFunction *function, XsLilSlotId slot);
size_t xs_lil_function_block_count(const XsLilFunction *function);
const XsLilBlock *xs_lil_function_block_at(const XsLilFunction *function, size_t index);
XsLilStatus xs_lil_function_append_block(XsLilFunction *function, const char *label, XsLilBlock **block,
                                         XsLilError *error);

#endif
