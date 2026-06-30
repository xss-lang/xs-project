#ifndef XS_HIR_TYPE_RESOLUTION_H
#define XS_HIR_TYPE_RESOLUTION_H

#include "xs/hir/symbol_table.h"

bool xs_hir_resolve_types(const XsSyntaxTree *tree, const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                          XsDiagnostics *diagnostics);

#endif
