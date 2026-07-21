/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef XS_LIL_C_TEXT_H
#define XS_LIL_C_TEXT_H

#include "xs/lil-c/module.h"

#include <stdio.h>

const char *xs_lil_type_name(XsLilType type);
XsLilStatus xs_lil_module_write_text(const XsLilModule *module, FILE *stream, XsLilError *error);

#endif
