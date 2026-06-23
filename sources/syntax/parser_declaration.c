#include "parser_internal.h"

static void parse_generics(SyntaxParser *parser, XsSyntaxNode *parent)
{
  if (!accept(parser, XS_TOKEN_LESS))
    return;
  while (parser->current.kind != XS_TOKEN_GREATER && parser->current.kind != XS_TOKEN_EOF) {
    size_t start = parser->current.span.start;
    XsSyntaxNode *parameter = node(parser, XS_SYNTAX_GENERIC_PARAMETER, (XsSpan){start, start});
    xs_syntax_node_add(parser->tree, parameter, identifier(parser));
    if (accept(parser, XS_TOKEN_COLON)) {
      do {
        xs_syntax_node_add(parser->tree, parameter, parse_type(parser));
        if (parser->current.kind != XS_TOKEN_COMMA || parser->next.kind == XS_TOKEN_IDENTIFIER)
          break;
        advance(parser);
      } while (true);
    }
    finish_node(parser, parameter, parser->previous.span.end);
    xs_syntax_node_add(parser->tree, parent, parameter);
    if (!accept(parser, XS_TOKEN_COMMA))
      break;
  }
  expect(parser, XS_TOKEN_GREATER, "expected '>' after generic parameters");
}

static void parse_parameters(SyntaxParser *parser, XsSyntaxNode *function)
{
  expect(parser, XS_TOKEN_LEFT_PAREN, "expected '(' before parameters");
  if (parser->current.kind != XS_TOKEN_RIGHT_PAREN) {
    do {
      size_t start = parser->current.span.start;
      XsSyntaxNode *parameter = node(parser, XS_SYNTAX_PARAMETER, (XsSpan){start, start});
      xs_syntax_node_add(parser->tree, parameter, identifier(parser));
      expect(parser, XS_TOKEN_COLON, "expected ':' after parameter name");
      xs_syntax_node_add(parser->tree, parameter, parse_type(parser));
      finish_node(parser, parameter, parser->previous.span.end);
      xs_syntax_node_add(parser->tree, function, parameter);
    } while (accept(parser, XS_TOKEN_COMMA));
  }
  expect(parser, XS_TOKEN_RIGHT_PAREN, "expected ')' after parameters");
}

static XsSyntaxNode *parse_function(SyntaxParser *parser, Modifiers modifiers, size_t start)
{
  XsSyntaxNode *function = node(parser, XS_SYNTAX_DECL_FUNCTION, (XsSpan){start, parser->previous.span.end});
  attach_modifiers(parser, function, modifiers);
  xs_syntax_node_add(parser->tree, function, identifier(parser));
  parse_generics(parser, function);
  parse_parameters(parser, function);
  if (accept(parser, XS_TOKEN_FAT_ARROW))
    xs_syntax_node_add(parser->tree, function, parse_type(parser));
  if (accept(parser, XS_TOKEN_KW_THROWS)) {
    do {
      xs_syntax_node_add(parser->tree, function, parse_type(parser));
    } while (accept(parser, XS_TOKEN_COMMA));
  }
  if (accept(parser, XS_TOKEN_SEMICOLON))
    function->flags |= XS_SYNTAX_FLAG_INCOMPLETE;
  else
    xs_syntax_node_add(parser->tree, function, parse_block(parser));
  finish_node(parser, function, parser->previous.span.end);
  return function;
}

static XsSyntaxNode *parse_import(SyntaxParser *parser, bool selected, size_t start)
{
  XsSyntaxNode *import = node(parser, XS_SYNTAX_DECL_IMPORT, (XsSpan){start, parser->previous.span.end});
  if (selected) {
    xs_syntax_node_add(parser->tree, import, parse_path(parser));
    expect(parser, XS_TOKEN_KW_IMPORTS, "expected imports after module path");
    if (accept(parser, XS_TOKEN_STAR)) {
      import->flags |= XS_SYNTAX_FLAG_WILDCARD;
    } else {
      do {
        XsSyntaxNode *name =
            node(parser, XS_SYNTAX_IMPORT_NAME, (XsSpan){parser->current.span.start, parser->current.span.start});
        xs_syntax_node_add(parser->tree, name, identifier(parser));
        if (accept(parser, XS_TOKEN_KW_AS))
          xs_syntax_node_add(parser->tree, name, identifier(parser));
        finish_node(parser, name, parser->previous.span.end);
        xs_syntax_node_add(parser->tree, import, name);
      } while (accept(parser, XS_TOKEN_COMMA));
    }
  } else {
    do {
      xs_syntax_node_add(parser->tree, import, parse_path(parser));
    } while (accept(parser, XS_TOKEN_COMMA));
    if (accept(parser, XS_TOKEN_KW_AS))
      xs_syntax_node_add(parser->tree, import, identifier(parser));
  }
  expect(parser, XS_TOKEN_SEMICOLON, "expected ';' after import");
  finish_node(parser, import, parser->previous.span.end);
  return import;
}

static XsSyntaxNode *parse_enum(SyntaxParser *parser, Modifiers modifiers, size_t start)
{
  XsSyntaxNode *declaration = node(parser, XS_SYNTAX_DECL_ENUM, (XsSpan){start, parser->previous.span.end});
  attach_modifiers(parser, declaration, modifiers);
  if (accept(parser, XS_TOKEN_KW_DATA))
    declaration->flags |= XS_SYNTAX_FLAG_DATA_ENUM;
  xs_syntax_node_add(parser->tree, declaration, identifier(parser));
  parse_generics(parser, declaration);
  expect(parser, XS_TOKEN_LEFT_BRACE, "expected '{' before enum variants");
  while (parser->current.kind != XS_TOKEN_RIGHT_BRACE && parser->current.kind != XS_TOKEN_EOF) {
    size_t start_variant = parser->current.span.start;
    XsSyntaxNode *variant = node(parser, XS_SYNTAX_ENUM_VARIANT, (XsSpan){start_variant, start_variant});
    xs_syntax_node_add(parser->tree, variant, identifier(parser));
    if (accept(parser, XS_TOKEN_COLON))
      xs_syntax_node_add(parser->tree, variant, parse_type(parser));
    finish_node(parser, variant, parser->previous.span.end);
    xs_syntax_node_add(parser->tree, declaration, variant);
    accept(parser, XS_TOKEN_COMMA);
  }
  expect(parser, XS_TOKEN_RIGHT_BRACE, "expected '}' after enum");
  finish_node(parser, declaration, parser->previous.span.end);
  return declaration;
}

static XsSyntaxNode *parse_data(SyntaxParser *parser, Modifiers modifiers, size_t start)
{
  XsSyntaxNode *declaration = node(parser, XS_SYNTAX_DECL_DATA, (XsSpan){start, parser->previous.span.end});
  attach_modifiers(parser, declaration, modifiers);
  xs_syntax_node_add(parser->tree, declaration, identifier(parser));
  parse_generics(parser, declaration);
  expect(parser, XS_TOKEN_LEFT_BRACE, "expected '{' before data fields");
  while (parser->current.kind != XS_TOKEN_RIGHT_BRACE && parser->current.kind != XS_TOKEN_EOF) {
    size_t start_field = parser->current.span.start;
    XsSyntaxNode *field = node(parser, XS_SYNTAX_DATA_FIELD, (XsSpan){start_field, start_field});
    xs_syntax_node_add(parser->tree, field, identifier(parser));
    expect(parser, XS_TOKEN_COLON, "expected ':' after data field name");
    xs_syntax_node_add(parser->tree, field, parse_type(parser));
    accept(parser, XS_TOKEN_SEMICOLON);
    finish_node(parser, field, parser->previous.span.end);
    xs_syntax_node_add(parser->tree, declaration, field);
  }
  expect(parser, XS_TOKEN_RIGHT_BRACE, "expected '}' after data declaration");
  finish_node(parser, declaration, parser->previous.span.end);
  return declaration;
}

static XsSyntaxNode *parse_class(SyntaxParser *parser, Modifiers modifiers, size_t start, bool interface)
{
  XsSyntaxNode *declaration = node(parser, interface ? XS_SYNTAX_DECL_INTERFACE : XS_SYNTAX_DECL_CLASS,
                                   (XsSpan){start, parser->previous.span.end});
  attach_modifiers(parser, declaration, modifiers);
  xs_syntax_node_add(parser->tree, declaration, identifier(parser));
  parse_generics(parser, declaration);
  expect(parser, XS_TOKEN_LEFT_BRACE, "expected '{' before declaration members");
  while (parser->current.kind != XS_TOKEN_RIGHT_BRACE && parser->current.kind != XS_TOKEN_EOF) {
    size_t before = parser->current.span.start;
    if (accept(parser, XS_TOKEN_KW_EXTENDS) || accept(parser, XS_TOKEN_KW_IMPLEMENTS)) {
      do {
        xs_syntax_node_add(parser->tree, declaration, parse_type(parser));
      } while (accept(parser, XS_TOKEN_COMMA));
      expect(parser, XS_TOKEN_SEMICOLON, "expected ';' after extends or implements");
    } else {
      Modifiers member = parse_modifiers(parser);
      if (accept(parser, XS_TOKEN_KW_FN))
        xs_syntax_node_add(parser->tree, declaration, parse_function(parser, member, before));
      else if (accept(parser, XS_TOKEN_KW_CLASS))
        xs_syntax_node_add(parser->tree, declaration, parse_class(parser, member, before, false));
      else if (accept(parser, XS_TOKEN_KW_ENUM))
        xs_syntax_node_add(parser->tree, declaration, parse_enum(parser, member, before));
      else if (accept(parser, XS_TOKEN_KW_DATA))
        xs_syntax_node_add(parser->tree, declaration, parse_data(parser, member, before));
      else if (accept(parser, XS_TOKEN_KW_MACRO_RULES))
        xs_syntax_node_add(parser->tree, declaration, parse_macro(parser, before));
      else if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_COLON) {
        XsSyntaxNode *field = parse_variable(parser, true);
        field->kind = XS_SYNTAX_CLASS_FIELD;
        field->visibility = member.visibility;
        field->flags |= member.flags;
        xs_syntax_node_add(parser->tree, declaration, field);
      } else if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_LEFT_PAREN) {
        XsSyntaxNode *constructor =
            node(parser, XS_SYNTAX_CLASS_CONSTRUCTOR, (XsSpan){before, parser->current.span.end});
        xs_syntax_node_add(parser->tree, constructor, identifier(parser));
        parse_parameters(parser, constructor);
        xs_syntax_node_add(parser->tree, constructor, parse_block(parser));
        finish_node(parser, constructor, parser->previous.span.end);
        xs_syntax_node_add(parser->tree, declaration, constructor);
      } else if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_DOT) {
        XsSyntaxNode *destructor = node(parser, XS_SYNTAX_CLASS_DESTRUCTOR, (XsSpan){before, parser->current.span.end});
        xs_syntax_node_add(parser->tree, destructor, identifier(parser));
        expect(parser, XS_TOKEN_DOT, "expected '.' in destructor declaration");
        if (parser->current.kind != XS_TOKEN_IDENTIFIER || !token_text_is(parser, parser->current, "Drop"))
          xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                             "destructor name must be Drop");
        xs_syntax_node_add(parser->tree, destructor, identifier(parser));
        expect(parser, XS_TOKEN_LEFT_PAREN, "expected '(' after destructor name");
        expect(parser, XS_TOKEN_RIGHT_PAREN, "destructors do not accept parameters");
        xs_syntax_node_add(parser->tree, destructor, parse_block(parser));
        finish_node(parser, destructor, parser->previous.span.end);
        xs_syntax_node_add(parser->tree, declaration, destructor);
      } else {
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                           "expected class or interface member");
        advance(parser);
      }
    }
    if (parser->current.span.start == before) {
      xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                         "parser made no progress in declaration members");
      advance(parser);
    }
  }
  expect(parser, XS_TOKEN_RIGHT_BRACE, "expected '}' after declaration members");
  finish_node(parser, declaration, parser->previous.span.end);
  return declaration;
}

XsSyntaxNode *parse_declaration(SyntaxParser *parser, bool top_level)
{
  size_t start = parser->current.span.start;
  Modifiers modifiers = parse_modifiers(parser);
  if (accept(parser, XS_TOKEN_KW_MODULE) || accept(parser, XS_TOKEN_KW_NAMESPACE)) {
    bool module = parser->previous.kind == XS_TOKEN_KW_MODULE;
    XsSyntaxNode *declaration = node(parser, module ? XS_SYNTAX_DECL_MODULE : XS_SYNTAX_DECL_NAMESPACE,
                                     (XsSpan){start, parser->previous.span.end});
    attach_modifiers(parser, declaration, modifiers);
    xs_syntax_node_add(parser->tree, declaration, parse_path(parser));
    expect(parser, XS_TOKEN_SEMICOLON, "expected ';' after module or namespace declaration");
    finish_node(parser, declaration, parser->previous.span.end);
    return declaration;
  }
  if (accept(parser, XS_TOKEN_KW_IMPORTS))
    return parse_import(parser, false, start);
  if (accept(parser, XS_TOKEN_KW_FROMS))
    return parse_import(parser, true, start);
  if (accept(parser, XS_TOKEN_KW_FN))
    return parse_function(parser, modifiers, start);
  if (accept(parser, XS_TOKEN_KW_CLASS))
    return parse_class(parser, modifiers, start, false);
  if (accept(parser, XS_TOKEN_KW_INTERFACE))
    return parse_class(parser, modifiers, start, true);
  if (accept(parser, XS_TOKEN_KW_ENUM))
    return parse_enum(parser, modifiers, start);
  if (accept(parser, XS_TOKEN_KW_DATA))
    return parse_data(parser, modifiers, start);
  if (accept(parser, XS_TOKEN_KW_MACRO_RULES))
    return parse_macro(parser, start);
  xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                     top_level ? "top-level execution is not allowed; expected a declaration" : "expected declaration");
  if (parser->current.kind != XS_TOKEN_EOF)
    advance(parser);
  return node(parser, XS_SYNTAX_TOKEN, (XsSpan){start, parser->previous.span.end});
}
