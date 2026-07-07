/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_SOURCE_INCLUDE_H
#define XS_SOURCE_INCLUDE_H

#include "xs/diagnostic.h"

typedef struct
{
  char *text;
  size_t length;
} XsIncludedSource;

bool xs_source_expand_includes(const XsSource *source, XsDiagnostics *diagnostics, XsIncludedSource *expanded);
void xs_included_source_free(XsIncludedSource *expanded);

#endif
