#include "parser_internal.h"

static XsSyntaxNode *parse_lifetime(SyntaxParser *parser)
{
  XsToken lifetime_token = parser->current;
  expect(parser, XS_TOKEN_LIFETIME, "expected lifetime");
  XsSyntaxNode *lifetime = node(parser, XS_SYNTAX_LIFETIME, lifetime_token.span);
  XsSpan name_span = {.start = lifetime_token.span.start + 1, .end = lifetime_token.span.end};
  xs_syntax_node_add(parser->tree, lifetime, node(parser, XS_SYNTAX_IDENTIFIER, name_span));
  return lifetime;
}

static XsSyntaxNode *parse_function_type(SyntaxParser *parser, size_t start)
{
  expect(parser, XS_TOKEN_KW_FN, "expected 'fn' in function type");
  XsSyntaxNode *function = node(parser, XS_SYNTAX_TYPE_FUNCTION, (XsSpan){start, parser->previous.span.end});
  expect(parser, XS_TOKEN_LEFT_PAREN, "expected '(' before function type parameters");
  if (parser->current.kind != XS_TOKEN_RIGHT_PAREN) {
    do {
      xs_syntax_node_add(parser->tree, function, parse_type(parser));
    } while (accept(parser, XS_TOKEN_COMMA));
  }
  expect(parser, XS_TOKEN_RIGHT_PAREN, "expected ')' after function type parameters");
  expect(parser, XS_TOKEN_FAT_ARROW, "expected '=>' before function type return type");
  xs_syntax_node_add(parser->tree, function, parse_type(parser));
  finish_node(parser, function, parser->previous.span.end);
  return function;
}

XsSyntaxNode *parse_type(SyntaxParser *parser)
{
  size_t start = parser->current.span.start;
  if (parser->current.kind == XS_TOKEN_KW_FN)
    return parse_function_type(parser, start);
  if (accept(parser, XS_TOKEN_AMPERSAND)) {
    XsSyntaxNode *lifetime = parser->current.kind == XS_TOKEN_LIFETIME ? parse_lifetime(parser) : NULL;
    bool mutable = accept(parser, XS_TOKEN_KW_MUT);
    XsSyntaxNode *reference = node(parser, mutable ? XS_SYNTAX_TYPE_MUTABLE_REFERENCE : XS_SYNTAX_TYPE_REFERENCE,
                                   (XsSpan){start, parser->previous.span.end});
    xs_syntax_node_add(parser->tree, reference, parse_type(parser));
    if (lifetime != NULL)
      xs_syntax_node_add(parser->tree, reference, lifetime);
    finish_node(parser, reference, parser->previous.span.end);
    return reference;
  }
  if (accept(parser, XS_TOKEN_STAR)) {
    XsSyntaxNode *pointer = node(parser, XS_SYNTAX_TYPE_POINTER, (XsSpan){start, parser->previous.span.end});
    xs_syntax_node_add(parser->tree, pointer, parse_type(parser));
    finish_node(parser, pointer, parser->previous.span.end);
    return pointer;
  }
  if (accept(parser, XS_TOKEN_LEFT_PAREN)) {
    if (accept(parser, XS_TOKEN_RIGHT_PAREN))
      return node(parser, XS_SYNTAX_TYPE_UNIT, (XsSpan){start, parser->previous.span.end});
    XsSyntaxNode *tuple = node(parser, XS_SYNTAX_TYPE_TUPLE, (XsSpan){start, start});
    xs_syntax_node_add(parser->tree, tuple, parse_type(parser));
    while (accept(parser, XS_TOKEN_COMMA))
      xs_syntax_node_add(parser->tree, tuple, parse_type(parser));
    expect(parser, XS_TOKEN_RIGHT_PAREN, "expected ')' after tuple type");
    finish_node(parser, tuple, parser->previous.span.end);
    return tuple;
  }
  if (parser->current.kind != XS_TOKEN_IDENTIFIER) {
    xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span, "expected type");
    if (parser->current.kind != XS_TOKEN_EOF)
      advance(parser);
    return node(parser, XS_SYNTAX_TYPE_NAMED, (XsSpan){start, parser->previous.span.end});
  }

  XsSyntaxNode *named = node(parser, XS_SYNTAX_TYPE_NAMED, (XsSpan){start, start});
  xs_syntax_node_add(parser->tree, named, parse_path(parser));
  finish_node(parser, named, parser->previous.span.end);
  XsSyntaxNode *result = named;
  if (accept(parser, XS_TOKEN_LESS)) {
    XsSyntaxNode *generic = node(parser, XS_SYNTAX_TYPE_GENERIC, (XsSpan){start, start});
    xs_syntax_node_add(parser->tree, generic, named);
    if (parser->current.kind != XS_TOKEN_GREATER) {
      do {
        xs_syntax_node_add(parser->tree, generic, parse_type(parser));
      } while (accept(parser, XS_TOKEN_COMMA));
    }
    expect(parser, XS_TOKEN_GREATER, "expected '>' after generic type arguments");
    finish_node(parser, generic, parser->previous.span.end);
    result = generic;
  }
  while (accept(parser, XS_TOKEN_LEFT_BRACKET)) {
    if (accept(parser, XS_TOKEN_RIGHT_BRACKET)) {
      XsSyntaxNode *array =
          node(parser, XS_SYNTAX_TYPE_ARRAY, (XsSpan){result->span.start_offset, parser->previous.span.end});
      xs_syntax_node_add(parser->tree, array, result);
      result = array;
    } else {
      XsSyntaxNode *array =
          node(parser, XS_SYNTAX_TYPE_FIXED_ARRAY, (XsSpan){result->span.start_offset, parser->current.span.start});
      xs_syntax_node_add(parser->tree, array, result);
      xs_syntax_node_add(parser->tree, array, parse_expression(parser, 1));
      expect(parser, XS_TOKEN_RIGHT_BRACKET, "expected ']' after fixed array maximum index");
      finish_node(parser, array, parser->previous.span.end);
      result = array;
    }
  }
  return result;
}
