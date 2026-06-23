#include "xs/syntax_parser.h"

#include "syntax/parser_internal.h"

#include <stddef.h>
#include <string.h>

static XsToken read_token(SyntaxParser *parser)
{
  XsToken token;
  do {
    token = xs_lexer_next(&parser->lexer);
  } while (token.kind == XS_TOKEN_DOC_COMMENT || token.kind == XS_TOKEN_MODULE_COMMENT);
  return token;
}

void advance(SyntaxParser *parser)
{
  parser->previous = parser->current;
  parser->current = parser->next;
  parser->next = read_token(parser);
}

bool accept(SyntaxParser *parser, XsTokenKind kind)
{
  if (parser->current.kind != kind)
    return false;
  advance(parser);
  return true;
}

bool expect(SyntaxParser *parser, XsTokenKind kind, const char *message)
{
  if (accept(parser, kind))
    return true;
  xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span, message);
  return false;
}

XsSyntaxNode *node(SyntaxParser *parser, XsSyntaxKind kind, XsSpan span)
{
  return xs_syntax_node_new(parser->tree, kind, span);
}

void finish_node(SyntaxParser *parser, XsSyntaxNode *value, size_t end)
{
  if (value == NULL)
    return;
  XsSpan span = {.start = value->span.start_offset, .end = end};
  value->span = xs_source_span(parser->tree, span);
  value->text = xs_source_text(parser->tree->source, span);
}

XsSyntaxNode *identifier(SyntaxParser *parser)
{
  if (!expect(parser, XS_TOKEN_IDENTIFIER, "expected identifier"))
    return NULL;
  return node(parser, XS_SYNTAX_IDENTIFIER, parser->previous.span);
}

XsSyntaxNode *parse_path(SyntaxParser *parser)
{
  size_t start = parser->current.span.start;
  XsSyntaxNode *path = node(parser, XS_SYNTAX_PATH, (XsSpan){start, start});
  XsSyntaxNode *segment = identifier(parser);
  if (segment == NULL)
    return path;
  xs_syntax_node_add(parser->tree, path, segment);
  while (accept(parser, XS_TOKEN_DOT)) {
    segment = identifier(parser);
    if (segment == NULL)
      break;
    xs_syntax_node_add(parser->tree, path, segment);
  }
  finish_node(parser, path, parser->previous.span.end);
  return path;
}

Modifiers parse_modifiers(SyntaxParser *parser)
{
  Modifiers result = {.visibility = XS_SYNTAX_VISIBILITY_DEFAULT,
                      .span = {parser->current.span.start, parser->current.span.start}};
  bool progress = true;
  while (progress) {
    progress = false;
    XsToken token = parser->current;
    if (accept(parser, XS_TOKEN_KW_PUBLIC)) {
      result.visibility = XS_SYNTAX_VISIBILITY_PUBLIC;
      progress = true;
    } else if (accept(parser, XS_TOKEN_KW_PRIVATE)) {
      result.visibility = XS_SYNTAX_VISIBILITY_PRIVATE;
      progress = true;
    } else if (accept(parser, XS_TOKEN_KW_PROTECTED)) {
      result.visibility = XS_SYNTAX_VISIBILITY_PROTECTED;
      progress = true;
    } else if (accept(parser, XS_TOKEN_KW_INTERNAL)) {
      result.visibility = XS_SYNTAX_VISIBILITY_INTERNAL;
      progress = true;
    } else if (accept(parser, XS_TOKEN_KW_ASYNC)) {
      result.flags |= XS_SYNTAX_FLAG_ASYNC;
      progress = true;
    } else if (accept(parser, XS_TOKEN_KW_STATIC)) {
      result.flags |= XS_SYNTAX_FLAG_STATIC;
      progress = true;
    } else if (accept(parser, XS_TOKEN_KW_INCOMPLETE)) {
      result.flags |= XS_SYNTAX_FLAG_INCOMPLETE;
      progress = true;
    }
    if (progress)
      result.span.end = token.span.end;
  }
  return result;
}

void attach_modifiers(SyntaxParser *parser, XsSyntaxNode *declaration, Modifiers modifiers)
{
  if (declaration == NULL)
    return;
  declaration->visibility = modifiers.visibility;
  declaration->flags |= modifiers.flags;
  XsSyntaxNode *visibility = node(parser, XS_SYNTAX_VISIBILITY, modifiers.span);
  if (visibility != NULL)
    visibility->visibility = modifiers.visibility;
  xs_syntax_node_add(parser->tree, declaration, visibility);
}

bool token_text_is(SyntaxParser *parser, XsToken token, const char *text)
{
  size_t length = token.span.end - token.span.start;
  return strlen(text) == length && memcmp(parser->tree->source->text + token.span.start, text, length) == 0;
}

bool xs_syntax_parse(const XsSource *source, uint64_t file_id, XsDiagnostics *diagnostics, XsSyntaxTree *tree)
{
  xs_syntax_tree_init(tree, source, file_id);
  SyntaxParser parser = {.diagnostics = diagnostics, .tree = tree};
  xs_lexer_init(&parser.lexer, source, diagnostics);
  parser.current = read_token(&parser);
  parser.next = read_token(&parser);
  tree->root = node(&parser, XS_SYNTAX_FILE, (XsSpan){0, source->length});

  bool seen_declaration = false;
  bool seen_module = false;
  while (parser.current.kind != XS_TOKEN_EOF) {
    size_t before = parser.current.span.start;
    XsSyntaxNode *declaration = parse_declaration(&parser, true);
    if (declaration != NULL) {
      if (declaration->kind == XS_SYNTAX_DECL_MODULE) {
        if (seen_declaration)
          xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR,
                             (XsSpan){declaration->span.start_offset, declaration->span.end_offset},
                             "module must be the first declaration in the file");
        if (seen_module)
          xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR,
                             (XsSpan){declaration->span.start_offset, declaration->span.end_offset},
                             "only one module declaration is allowed per file");
        seen_module = true;
      }
      if (declaration->kind == XS_SYNTAX_DECL_NAMESPACE && !seen_module)
        xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR,
                           (XsSpan){declaration->span.start_offset, declaration->span.end_offset},
                           "namespace must be declared under module");
      xs_syntax_node_add(tree, tree->root, declaration);
      seen_declaration = true;
    }
    if (parser.current.span.start == before) {
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, parser.current.span, "parser made no progress at top level");
      advance(&parser);
    }
  }
  if (tree->allocation_failed)
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while building structural AST");
  return !xs_diagnostics_has_error(diagnostics);
}
