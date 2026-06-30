#ifndef XS_HIR_TYPE_RESOLUTION_H
#define XS_HIR_TYPE_RESOLUTION_H

#include "xs/hir/symbol_table.h"

bool xs_hir_resolve_types(const XsSyntaxTree *tree, const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                          XsDiagnostics *diagnostics);

bool xs_hir_resolve_types_expanded(const XsSyntaxTree *tree, const XsMacroStatementExpansionSet *macro_statements,
                                   const XsHirSymbolTable *symbols, const XsHirImportScope *imports,
                                   XsDiagnostics *diagnostics);

#endif
