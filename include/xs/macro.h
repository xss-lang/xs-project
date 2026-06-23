#ifndef XS_MACRO_H
#define XS_MACRO_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

bool xs_macro_validate(const XsSyntaxTree *tree, XsDiagnostics *diagnostics);

#endif
