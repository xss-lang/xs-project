/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_DRIVER_DIRECT_XLIL_H
#define XS_DRIVER_DIRECT_XLIL_H

#include <stddef.h>

bool xs_driver_emit_direct_xlil_llvm_ir(const char *input_path, const char *text, size_t length);

#endif
