/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XS_DIAGNOSTIC_H
#define XS_DIAGNOSTIC_H

#include "xs/source.h"

#include <stddef.h>
#include <stdio.h>

typedef enum
{
  XS_DIAGNOSTIC_ERROR,
  XS_DIAGNOSTIC_WARNING,
  XS_DIAGNOSTIC_NOTE,
} XsDiagnosticSeverity;

typedef struct
{
  XsDiagnosticSeverity severity;
  XsSpan span;
  char *message;
} XsDiagnostic;

typedef struct
{
  XsDiagnostic *items;
  size_t count;
  size_t capacity;
  bool allocation_failed;
} XsDiagnostics;

void xs_diagnostics_init(XsDiagnostics *diagnostics);
void xs_diagnostics_free(XsDiagnostics *diagnostics);
bool xs_diagnostics_add(XsDiagnostics *diagnostics, XsDiagnosticSeverity severity, XsSpan span, const char *message);
bool xs_diagnostics_has_error(const XsDiagnostics *diagnostics);
void xs_diagnostics_print(const XsDiagnostics *diagnostics, const XsSource *source, FILE *stream);

#endif
