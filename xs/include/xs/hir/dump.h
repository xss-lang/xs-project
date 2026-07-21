/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef XS_HIR_DUMP_H
#define XS_HIR_DUMP_H

#include "xs/hir/symbol_table.h"

#include <stdio.h>

bool xs_hir_write_symbols(const XsHirSymbolTable *symbols, FILE *stream);

#endif
