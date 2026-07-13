/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_LLVM_STRING_H
#define XS_LLVM_STRING_H

#include "xs/backend/llvm_backend.h"

XsBackendStatus xs_llvm_lower_lil_const_str(XsLlvmBackend *backend, XsLlvmCodegenUnit *unit, const XsLilBlock *block,
                                            size_t index, LLVMValueRef *value, XsBackendError *error);

#endif
