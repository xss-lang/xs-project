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

void xs_macro_reparse_result_free(XsMacroReparseResult *result)
{
  xs_syntax_tree_free(&result->tree);
  free(result->text);
  *result = (XsMacroReparseResult){0};
}
