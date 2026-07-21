/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef XS_BACKEND_LINKER_H
#define XS_BACKEND_LINKER_H

#include "xs/backend/llvm_backend.h"

#include <stddef.h>

typedef struct
{
  const char *program;
  const char *const *arguments;
  size_t argument_count;
} XsLinkerInvocation;

XsBackendStatus xs_linker_invoke(const XsLinkerInvocation *invocation, int *exit_code, XsBackendError *error);

#endif
