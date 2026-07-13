/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_BACKEND_LLVM_INTEGER_H
#define XS_BACKEND_LLVM_INTEGER_H

#include "xs/backend/llvm_backend.h"

bool xs_llvm_is_integer_constant(XsLilInstructionKind kind);
XsBackendStatus xs_llvm_lower_integer_constant(XsLlvmBackend *backend, const XsLilBlock *block, size_t index,
                                               LLVMValueRef *value, XsBackendError *error);

#endif
