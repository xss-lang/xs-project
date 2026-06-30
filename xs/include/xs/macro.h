#ifndef XS_MACRO_H
#define XS_MACRO_H

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"

#include <stdint.h>

typedef struct
{
  size_t calls_seen;
  size_t calls_resolved;
  size_t calls_expandable;
  size_t calls_deferred;
  size_t output_tokens_planned;
  size_t substitutions_planned;
} XsMacroExpansionReport;

typedef struct
{
  XsTokenKind kind;
  XsText text;
  XsSpan source_span;
  bool from_substitution;
} XsMacroExpansionToken;

typedef struct
{
  XsSpan call_span;
  XsMacroExpansionToken *tokens;
  size_t token_count;
  size_t token_capacity;
} XsMacroExpansion;

typedef struct
{
  XsMacroExpansion *items;
  size_t count;
  size_t capacity;
  bool allocation_failed;
} XsMacroExpansionSet;

typedef struct
{
  char *text;
  XsSource source;
  XsSyntaxTree tree;
} XsMacroReparseResult;

typedef struct
{
  XsSpan call_span;
  XsMacroReparseResult reparse;
  const XsSyntaxNode *statement;
} XsMacroStatementExpansion;

typedef struct
{
  XsMacroStatementExpansion *items;
  size_t count;
  size_t capacity;
  bool allocation_failed;
} XsMacroStatementExpansionSet;

bool xs_macro_validate(const XsSyntaxTree *tree, XsDiagnostics *diagnostics);
bool xs_macro_prepare_expansion(const XsSyntaxTree *tree, XsDiagnostics *diagnostics, XsMacroExpansionReport *report);
void xs_macro_expansion_set_free(XsMacroExpansionSet *set);
bool xs_macro_expand_tokens(const XsSyntaxTree *tree, XsDiagnostics *diagnostics, XsMacroExpansionSet *set);
bool xs_macro_reparse_expansion_as_statement(const XsMacroExpansion *expansion, uint64_t file_id,
                                             XsDiagnostics *diagnostics, XsMacroReparseResult *result);
const XsSyntaxNode *xs_macro_reparse_result_statement(const XsMacroReparseResult *result);
void xs_macro_reparse_result_free(XsMacroReparseResult *result);
void xs_macro_statement_expansion_set_free(XsMacroStatementExpansionSet *set);
bool xs_macro_expand_statements(const XsSyntaxTree *tree, XsDiagnostics *diagnostics,
                                XsMacroStatementExpansionSet *set);

#endif
