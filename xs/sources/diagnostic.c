/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/diagnostic.h"

#include <stdlib.h>
#include <string.h>

void xs_diagnostics_init(XsDiagnostics *diagnostics)
{
  *diagnostics = (XsDiagnostics){0};
}

void xs_diagnostics_free(XsDiagnostics *diagnostics)
{
  for (size_t i = 0; i < diagnostics->count; ++i)
    free(diagnostics->items[i].message);
  free(diagnostics->items);
  *diagnostics = (XsDiagnostics){0};
}

bool xs_diagnostics_add(XsDiagnostics *diagnostics, XsDiagnosticSeverity severity, XsSpan span, const char *message)
{
  if (diagnostics->count == diagnostics->capacity)
  {
    size_t capacity = diagnostics->capacity == 0 ? 8 : diagnostics->capacity * 2;
    XsDiagnostic *items = realloc(diagnostics->items, capacity * sizeof(*items));
    if (items == NULL)
    {
      diagnostics->allocation_failed = true;
      return false;
    }
    diagnostics->items = items;
    diagnostics->capacity = capacity;
  }

  size_t length = strlen(message);
  char *copy = malloc(length + 1);
  if (copy == NULL)
  {
    diagnostics->allocation_failed = true;
    return false;
  }
  memcpy(copy, message, length + 1);
  diagnostics->items[diagnostics->count++] = (XsDiagnostic){.severity = severity, .span = span, .message = copy};
  return true;
}

bool xs_diagnostics_has_error(const XsDiagnostics *diagnostics)
{
  for (size_t i = 0; i < diagnostics->count; ++i)
  {
    if (diagnostics->items[i].severity == XS_DIAGNOSTIC_ERROR)
      return true;
  }
  return diagnostics->allocation_failed;
}

void xs_diagnostics_print(const XsDiagnostics *diagnostics, const XsSource *source, FILE *stream)
{
  static const char *const severities[] = {"error", "warning", "note"};
  for (size_t i = 0; i < diagnostics->count; ++i)
  {
    XsDiagnostic diagnostic = diagnostics->items[i];
    size_t line = 1;
    size_t column = 1;
    size_t limit = diagnostic.span.start < source->length ? diagnostic.span.start : source->length;
    for (size_t offset = 0; offset < limit; ++offset)
    {
      if (source->text[offset] == '\n')
      {
        ++line;
        column = 1;
      }
      else
      {
        ++column;
      }
    }
    fprintf(stream, "%s:%zu:%zu: %s: %s\n", source->path, line, column, severities[diagnostic.severity],
            diagnostic.message);
  }
  if (diagnostics->allocation_failed)
    fprintf(stream, "%s: error: compiler ran out of memory while recording diagnostics\n", source->path);
}
