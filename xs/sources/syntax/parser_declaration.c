#include "parser_internal.h"

#include <string.h>

static bool syntax_text_equal(XsText left, XsText right)
{
  return left.length == right.length && memcmp(left.data, right.data, left.length) == 0;
}

static bool generic_constraint_comma_starts_parameter(SyntaxParser *parser)
{
  if (parser->current.kind != XS_TOKEN_COMMA || parser->next.kind != XS_TOKEN_IDENTIFIER)
    return false;
  SyntaxParser lookahead = *parser;
  advance(&lookahead);
  advance(&lookahead);
  return lookahead.current.kind == XS_TOKEN_COLON;
}

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
        if (parser->current.kind != XS_TOKEN_COMMA || generic_constraint_comma_starts_parameter(parser))
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

static XsSyntaxNode *parse_function(SyntaxParser *parser, Modifiers modifiers, size_t start, bool signature_allowed)
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
  if (accept(parser, XS_TOKEN_SEMICOLON)) {
    if (!signature_allowed && (function->flags & XS_SYNTAX_FLAG_INCOMPLETE) == 0)
      xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->previous.span,
                         "function declarations without bodies must be marked incomplete");
    function->flags |= XS_SYNTAX_FLAG_INCOMPLETE;
  } else {
    if ((function->flags & XS_SYNTAX_FLAG_INCOMPLETE) != 0)
      xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                         "incomplete functions must not have bodies");
    xs_syntax_node_add(parser->tree, function, parse_block(parser));
  }
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

static XsSyntaxNode *parse_declaration_macro_call(SyntaxParser *parser, size_t start)
{
  XsSyntaxNode *declaration = node(parser, XS_SYNTAX_DECL_MACRO_CALL, (XsSpan){start, start});
  xs_syntax_node_add(parser->tree, declaration, parse_expression(parser, 1));
  expect(parser, XS_TOKEN_SEMICOLON, "expected ';' after macro call declaration");
  finish_node(parser, declaration, parser->previous.span.end);
  return declaration;
}

static XsSyntaxNode *parse_enum(SyntaxParser *parser, Modifiers modifiers, size_t start)
{
  XsSyntaxNode *declaration = node(parser, XS_SYNTAX_DECL_ENUM, (XsSpan){start, parser->previous.span.end});
  attach_modifiers(parser, declaration, modifiers);
  bool data_enum = accept(parser, XS_TOKEN_KW_DATA);
  if (data_enum)
    declaration->flags |= XS_SYNTAX_FLAG_DATA_ENUM;
  xs_syntax_node_add(parser->tree, declaration, identifier(parser));
  parse_generics(parser, declaration);
  expect(parser, XS_TOKEN_LEFT_BRACE, "expected '{' before enum variants");
  bool has_payload_variant = false;
  while (parser->current.kind != XS_TOKEN_RIGHT_BRACE && parser->current.kind != XS_TOKEN_EOF) {
    size_t start_variant = parser->current.span.start;
    XsSyntaxNode *variant = node(parser, XS_SYNTAX_ENUM_VARIANT, (XsSpan){start_variant, start_variant});
    xs_syntax_node_add(parser->tree, variant, identifier(parser));
    if (accept(parser, XS_TOKEN_COLON)) {
      XsSpan colon_span = parser->previous.span;
      has_payload_variant = true;
      if (!data_enum)
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, colon_span,
                           "regular enum variants cannot contain payload types");
      XsSyntaxNode *payload = parse_type(parser);
      if (data_enum && payload != NULL && payload->kind == XS_SYNTAX_TYPE_TUPLE)
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR,
                           (XsSpan){payload->span.start_offset, payload->span.end_offset},
                           "data enum tuple payloads are not supported");
      xs_syntax_node_add(parser->tree, variant, payload);
    }
    finish_node(parser, variant, parser->previous.span.end);
    xs_syntax_node_add(parser->tree, declaration, variant);
    accept(parser, XS_TOKEN_COMMA);
  }
  expect(parser, XS_TOKEN_RIGHT_BRACE, "expected '}' after enum");
  if (data_enum && !has_payload_variant)
    xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){start, parser->previous.span.end},
                       "enum data declarations require at least one typed variant");
  finish_node(parser, declaration, parser->previous.span.end);
  return declaration;
}

static bool data_member_is_forbidden(const SyntaxParser *parser)
{
  return parser->current.kind == XS_TOKEN_KW_FN || parser->current.kind == XS_TOKEN_KW_EXTENDS ||
         parser->current.kind == XS_TOKEN_KW_IMPLEMENTS ||
         (parser->current.kind == XS_TOKEN_IDENTIFIER &&
          (parser->next.kind == XS_TOKEN_LEFT_PAREN || parser->next.kind == XS_TOKEN_DOT));
}

static void skip_forbidden_data_member(SyntaxParser *parser)
{
  size_t depth = 0;
  do {
    if (parser->current.kind == XS_TOKEN_LEFT_BRACE || parser->current.kind == XS_TOKEN_LEFT_PAREN ||
        parser->current.kind == XS_TOKEN_LEFT_BRACKET)
      ++depth;
    else if (parser->current.kind == XS_TOKEN_RIGHT_BRACE || parser->current.kind == XS_TOKEN_RIGHT_PAREN ||
             parser->current.kind == XS_TOKEN_RIGHT_BRACKET) {
      if (depth == 0)
        return;
      --depth;
    }
    advance(parser);
    if (depth == 0 && parser->previous.kind == XS_TOKEN_SEMICOLON)
      return;
  } while (parser->current.kind != XS_TOKEN_EOF && parser->current.kind != XS_TOKEN_RIGHT_BRACE);
}

static XsSyntaxNode *parse_data(SyntaxParser *parser, Modifiers modifiers, size_t start)
{
  XsSyntaxNode *declaration = node(parser, XS_SYNTAX_DECL_DATA, (XsSpan){start, parser->previous.span.end});
  attach_modifiers(parser, declaration, modifiers);
  xs_syntax_node_add(parser->tree, declaration, identifier(parser));
  parse_generics(parser, declaration);
  expect(parser, XS_TOKEN_LEFT_BRACE, "expected '{' before data fields");
  while (parser->current.kind != XS_TOKEN_RIGHT_BRACE && parser->current.kind != XS_TOKEN_EOF) {
    if (data_member_is_forbidden(parser)) {
      xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                         "data declarations may only contain fields");
      skip_forbidden_data_member(parser);
      continue;
    }
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
  XsSyntaxNode *class_name = identifier(parser);
  xs_syntax_node_add(parser->tree, declaration, class_name);
  parse_generics(parser, declaration);
  expect(parser, XS_TOKEN_LEFT_BRACE, "expected '{' before declaration members");
  bool constructor_seen = false;
  bool extends_seen = false;
  while (parser->current.kind != XS_TOKEN_RIGHT_BRACE && parser->current.kind != XS_TOKEN_EOF) {
    size_t before = parser->current.span.start;
    if (accept(parser, XS_TOKEN_KW_EXTENDS)) {
      if (interface) {
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->previous.span,
                           "interfaces may only contain function declarations");
        skip_forbidden_data_member(parser);
        continue;
      }
      if (extends_seen)
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->previous.span,
                           "a class may contain at most one extends declaration");
      extends_seen = true;
      xs_syntax_node_add(parser->tree, declaration, parse_type(parser));
      if (accept(parser, XS_TOKEN_COMMA)) {
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->previous.span,
                           "extends accepts exactly one base class");
        do {
          (void)parse_type(parser);
        } while (accept(parser, XS_TOKEN_COMMA));
      }
      expect(parser, XS_TOKEN_SEMICOLON, "expected ';' after extends or implements");
    } else if (accept(parser, XS_TOKEN_KW_IMPLEMENTS)) {
      if (interface) {
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->previous.span,
                           "interfaces may only contain function declarations");
        skip_forbidden_data_member(parser);
        continue;
      }
      do {
        xs_syntax_node_add(parser->tree, declaration, parse_type(parser));
      } while (accept(parser, XS_TOKEN_COMMA));
      expect(parser, XS_TOKEN_SEMICOLON, "expected ';' after extends or implements");
    } else {
      Modifiers member = parse_modifiers(parser);
      if (accept(parser, XS_TOKEN_KW_FN)) {
        XsSyntaxNode *function = parse_function(parser, member, before, interface);
        if (interface && (function->flags & XS_SYNTAX_FLAG_INCOMPLETE) == 0) {
          XsSpan function_span = {function->span.start_offset, function->span.end_offset};
          xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, function_span,
                             "interface methods must be declarations without bodies");
        }
        xs_syntax_node_add(parser->tree, declaration, function);
      } else if (interface) {
        xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                           "interfaces may only contain function declarations");
        skip_forbidden_data_member(parser);
      } else if (accept(parser, XS_TOKEN_KW_CLASS))
        xs_syntax_node_add(parser->tree, declaration, parse_class(parser, member, before, false));
      else if (accept(parser, XS_TOKEN_KW_ENUM))
        xs_syntax_node_add(parser->tree, declaration, parse_enum(parser, member, before));
      else if (accept(parser, XS_TOKEN_KW_DATA))
        xs_syntax_node_add(parser->tree, declaration, parse_data(parser, member, before));
      else if (accept(parser, XS_TOKEN_KW_MACRO_RULES))
        xs_syntax_node_add(parser->tree, declaration, parse_macro(parser, before));
      else if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_BANG)
        xs_syntax_node_add(parser->tree, declaration, parse_declaration_macro_call(parser, before));
      else if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_COLON) {
        XsSyntaxNode *field = parse_variable(parser, true);
        field->kind = XS_SYNTAX_CLASS_FIELD;
        field->visibility = member.visibility;
        field->flags |= member.flags;
        xs_syntax_node_add(parser->tree, declaration, field);
      } else if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_LEFT_PAREN) {
        XsSyntaxNode *constructor =
            node(parser, XS_SYNTAX_CLASS_CONSTRUCTOR, (XsSpan){before, parser->current.span.end});
        XsSyntaxNode *constructor_name = identifier(parser);
        XsSpan constructor_span =
            constructor_name == NULL ? parser->previous.span
                                     : (XsSpan){constructor_name->span.start_offset, constructor_name->span.end_offset};
        if (class_name != NULL && constructor_name != NULL &&
            !syntax_text_equal(class_name->text, constructor_name->text))
          xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, constructor_span,
                             "constructor name must match the class name");
        if (constructor_seen)
          xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, constructor_span,
                             "a class may contain at most one constructor");
        constructor_seen = true;
        xs_syntax_node_add(parser->tree, constructor, constructor_name);
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
    return parse_function(parser, modifiers, start, false);
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
  if (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_BANG)
    return parse_declaration_macro_call(parser, start);
  if (parser->current.kind == XS_TOKEN_KW_VAL || parser->current.kind == XS_TOKEN_KW_CONST ||
      parser->current.kind == XS_TOKEN_KW_ATOMIC ||
      (parser->current.kind == XS_TOKEN_IDENTIFIER && parser->next.kind == XS_TOKEN_COLON)) {
    XsSyntaxNode *variable = parse_variable(parser, true);
    attach_modifiers(parser, variable, modifiers);
    return variable;
  }
  xs_diagnostics_add(parser->diagnostics, XS_DIAGNOSTIC_ERROR, parser->current.span,
                     top_level ? "top-level execution is not allowed; expected a declaration" : "expected declaration");
  if (parser->current.kind != XS_TOKEN_EOF)
    advance(parser);
  return node(parser, XS_SYNTAX_TOKEN, (XsSpan){start, parser->previous.span.end});
}
