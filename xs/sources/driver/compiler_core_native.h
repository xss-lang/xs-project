/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_DRIVER_COMPILER_CORE_NATIVE_H
#define XS_DRIVER_COMPILER_CORE_NATIVE_H

#include "xs/compiler_core.h"
#include "xs/diagnostic.h"

bool xs_driver_compiler_core_native_available(const XsCompilerCoreSession *session);
bool xs_driver_build_compiler_core_native(const char *input_path, const XsCompilerCoreSession *session,
                                          XsDiagnostics *diagnostics, XsSpan span);

#endif
