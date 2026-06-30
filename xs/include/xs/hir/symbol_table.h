#ifndef XS_HIR_SYMBOL_TABLE_H
#define XS_HIR_SYMBOL_TABLE_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

#include <stddef.h>

typedef enum
{
  XS_HIR_SYMBOL_FUNCTION,
  XS_HIR_SYMBOL_CLASS,
  XS_HIR_SYMBOL_INTERFACE,
  XS_HIR_SYMBOL_ENUM,
  XS_HIR_SYMBOL_DATA,
  XS_HIR_SYMBOL_MACRO,
} XsHirSymbolKind;

typedef struct
{
  XsHirSymbolKind kind;
  char *name;
  char *namespace_name;
  char *qualified_name;
  XsSyntaxVisibility visibility;
  XsSourceSpan span;
  const XsSyntaxNode *syntax;
} XsHirSymbol;

typedef struct
{
  char *local_name;
  const XsHirSymbol *symbol;
  XsSourceSpan span;
} XsHirImportBinding;

typedef struct
{
  XsHirSymbol *symbols;
  size_t count;
  size_t capacity;
  bool allocation_failed;
} XsHirSymbolTable;

typedef struct
{
  XsHirImportBinding *bindings;
  size_t count;
  size_t capacity;
  bool allocation_failed;
} XsHirImportScope;

void xs_hir_symbol_table_init(XsHirSymbolTable *table);
void xs_hir_symbol_table_free(XsHirSymbolTable *table);
const XsHirSymbol *xs_hir_symbol_table_find(const XsHirSymbolTable *table, const char *qualified_name);
bool xs_hir_collect_symbols(const XsSyntaxTree *tree, XsHirSymbolTable *table, XsDiagnostics *diagnostics);
const char *xs_hir_symbol_kind_name(XsHirSymbolKind kind);

void xs_hir_import_scope_init(XsHirImportScope *scope);
void xs_hir_import_scope_free(XsHirImportScope *scope);
const XsHirImportBinding *xs_hir_import_scope_find(const XsHirImportScope *scope, const char *local_name);
bool xs_hir_resolve_imports(const XsSyntaxTree *tree, const XsHirSymbolTable *project_symbols, XsHirImportScope *scope,
                            XsDiagnostics *diagnostics);
bool xs_hir_validate_name_uses(const XsSyntaxTree *tree, const XsHirSymbolTable *project_symbols,
                               const XsHirImportScope *imports, XsDiagnostics *diagnostics);

#endif
