#include "xs/macro.h"
#include "xs/syntax_parser.h"

#include <stdlib.h>
#include <string.h>

static bool append_text(char *buffer, size_t *cursor, const char *text, size_t length)
{
  if (length == 0)
    return true;
  memcpy(buffer + *cursor, text, length);
  *cursor += length;
  return true;
}

static size_t expansion_text_length(const XsMacroExpansion *expansion)
{
  size_t length = 0;
  for (size_t i = 0; i < expansion->token_count; ++i)
    length += expansion->tokens[i].text.length + 1;
  return length;
}

static char *render_statement_source(const XsMacroExpansion *expansion, size_t *length)
{
  static const char prefix[] = "fn __xs_macro_expansion_probe() { ";
  static const char suffix[] = "; }\n";
  size_t capacity = sizeof(prefix) - 1 + expansion_text_length(expansion) + sizeof(suffix);
  char *text = malloc(capacity);
  if (text == NULL)
    return NULL;

  size_t cursor = 0;
  append_text(text, &cursor, prefix, sizeof(prefix) - 1);
  for (size_t i = 0; i < expansion->token_count; ++i) {
    if (i != 0)
      append_text(text, &cursor, " ", 1);
    append_text(text, &cursor, expansion->tokens[i].text.data, expansion->tokens[i].text.length);
  }
  append_text(text, &cursor, suffix, sizeof(suffix) - 1);
  text[cursor] = '\0';
  *length = cursor;
  return text;
}

bool xs_macro_reparse_expansion_as_statement(const XsMacroExpansion *expansion, uint64_t file_id,
                                             XsDiagnostics *diagnostics, XsMacroReparseResult *result)
{
  if (expansion == NULL || diagnostics == NULL || result == NULL || expansion->token_count == 0)
    return false;
  *result = (XsMacroReparseResult){0};
  size_t length = 0;
  char *text = render_statement_source(expansion, &length);
  if (text == NULL) {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, expansion->call_span,
                       "compiler ran out of memory while reparsing macro expansion");
    return false;
  }
  result->text = text;
  result->source = (XsSource){.path = "<macro-expansion>", .text = text, .length = length};
  if (!xs_syntax_parse(&result->source, file_id, diagnostics, &result->tree)) {
    xs_macro_reparse_result_free(result);
    return false;
  }
  return true;
}

const XsSyntaxNode *xs_macro_reparse_result_statement(const XsMacroReparseResult *result)
{
  const XsSyntaxNode *function = xs_syntax_find_first(result == NULL ? NULL : result->tree.root, XS_SYNTAX_DECL_FUNCTION);
  const XsSyntaxNode *block = xs_syntax_find_first(function, XS_SYNTAX_STMT_BLOCK);
  if (block == NULL || block->child_count == 0)
    return NULL;
  return block->children[0];
}

void xs_macro_reparse_result_free(XsMacroReparseResult *result)
{
  xs_syntax_tree_free(&result->tree);
  free(result->text);
  *result = (XsMacroReparseResult){0};
}

static bool statement_set_add(XsMacroStatementExpansionSet *set, XsMacroStatementExpansion item)
{
  if (set->count == set->capacity) {
    size_t capacity = set->capacity == 0 ? 4 : set->capacity * 2;
    XsMacroStatementExpansion *items = realloc(set->items, capacity * sizeof(*items));
    if (items == NULL) {
      set->allocation_failed = true;
      return false;
    }
    set->items = items;
    set->capacity = capacity;
  }
  set->items[set->count++] = item;
  return true;
}

void xs_macro_statement_expansion_set_free(XsMacroStatementExpansionSet *set)
{
  for (size_t i = 0; i < set->count; ++i)
    xs_macro_reparse_result_free(&set->items[i].reparse);
  free(set->items);
  *set = (XsMacroStatementExpansionSet){0};
}

bool xs_macro_expand_statements(const XsSyntaxTree *tree, XsDiagnostics *diagnostics, XsMacroStatementExpansionSet *set)
{
  if (tree == NULL || tree->root == NULL || diagnostics == NULL || set == NULL)
    return false;
  *set = (XsMacroStatementExpansionSet){0};

  XsMacroExpansionSet tokens = {0};
  if (!xs_macro_expand_tokens(tree, diagnostics, &tokens)) {
    xs_macro_expansion_set_free(&tokens);
    return false;
  }

  for (size_t i = 0; i < tokens.count; ++i) {
    XsMacroReparseResult reparse;
    if (!xs_macro_reparse_expansion_as_statement(&tokens.items[i], tree->file_id, diagnostics, &reparse)) {
      xs_macro_statement_expansion_set_free(set);
      xs_macro_expansion_set_free(&tokens);
      return false;
    }
    const XsSyntaxNode *statement = xs_macro_reparse_result_statement(&reparse);
    if (statement == NULL) {
      xs_macro_reparse_result_free(&reparse);
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, tokens.items[i].call_span,
                         "macro expansion did not produce a statement");
      xs_macro_statement_expansion_set_free(set);
      xs_macro_expansion_set_free(&tokens);
      return false;
    }
    XsMacroStatementExpansion item = {
        .call_span = tokens.items[i].call_span,
        .reparse = reparse,
        .statement = statement,
    };
    if (!statement_set_add(set, item)) {
      xs_macro_reparse_result_free(&reparse);
      xs_macro_statement_expansion_set_free(set);
      xs_macro_expansion_set_free(&tokens);
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, tokens.items[i].call_span,
                         "compiler ran out of memory while recording macro statement expansion");
      return false;
    }
  }

  xs_macro_expansion_set_free(&tokens);
  return !set->allocation_failed && !xs_diagnostics_has_error(diagnostics);
}
