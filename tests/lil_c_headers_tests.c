/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/lil-c/function.h"
#include "xs/lil-c/instruction.h"
#include "xs/lil-c/model.h"
#include "xs/lil-c/module.h"
#include "xs/lil-c/text.h"

int main(void)
{
  XsLilError error = {0};
  XsLilModule *module = nullptr;
  if(xs_lil_module_create("PublicCProducer", &module, &error) != XS_LIL_OK)
    return 1;
  XsLilFunction *function = nullptr;
  if(xs_lil_module_add_function_definition(module, "main", (XsLilType){.kind = XS_LIL_TYPE_I32}, nullptr, 0,
                                                &function, &error) != XS_LIL_OK)
  {
    xs_lil_module_destroy(module);
    return 2;
  }
  XsLilBlock *entry = nullptr;
  XsLilValueId result = 0;
  bool valid = xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK &&
               xs_lil_block_add_const_i32(entry, 0, &result, &error) == XS_LIL_OK &&
               xs_lil_block_set_return_value(entry, result, &error) == XS_LIL_OK &&
               xs_lil_module_verify(module, &error) == XS_LIL_OK;
  xs_lil_module_destroy(module);
  return valid ? 0 : 3;
}
