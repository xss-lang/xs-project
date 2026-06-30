#ifndef XS_MACRO_H
#define XS_MACRO_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

typedef struct
{
  size_t calls_seen;
  size_t calls_resolved;
  size_t calls_expandable;
  size_t calls_deferred;
} XsMacroExpansionReport;

bool xs_macro_validate(const XsSyntaxTree *tree, XsDiagnostics *diagnostics);
bool xs_macro_prepare_expansion(const XsSyntaxTree *tree, XsDiagnostics *diagnostics, XsMacroExpansionReport *report);

#endif
