/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_DRIVER_SOURCE_NATIVE_H
#define XS_DRIVER_SOURCE_NATIVE_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

bool xs_driver_build_source_native(const char *input_path, const XsSyntaxTree *tree, XsDiagnostics *diagnostics);

#endif
