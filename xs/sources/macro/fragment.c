#include "internal.h"

#include <string.h>

bool xs_macro_text_equal(XsText left, XsText right)
{
  return left.length == right.length && memcmp(left.data, right.data, left.length) == 0;
}

bool xs_macro_token_text_matches(const XsSyntaxNode *matcher, const XsSyntaxNode *argument)
{
  if (matcher->token_kind != argument->token_kind)
    return false;
  if (matcher->text.length == 0 || argument->text.length == 0)
    return true;
  return xs_macro_text_equal(matcher->text, argument->text);
}

static bool token_is_literal(XsTokenKind kind)
{
  return kind == XS_TOKEN_INTEGER || kind == XS_TOKEN_FLOAT || kind == XS_TOKEN_STRING || kind == XS_TOKEN_CHARACTER ||
         kind == XS_TOKEN_KW_TRUE || kind == XS_TOKEN_KW_FALSE || kind == XS_TOKEN_KW_NIL;
}

static bool token_is_visibility(XsTokenKind kind)
{
  return kind == XS_TOKEN_KW_PUBLIC || kind == XS_TOKEN_KW_PRIVATE || kind == XS_TOKEN_KW_PROTECTED ||
         kind == XS_TOKEN_KW_INTERNAL;
}

static bool single_token_fragment_supported(XsText kind)
{
  return xs_macro_text_equal(kind, (XsText){.data = "tt", .length = 2}) ||
         xs_macro_text_equal(kind, (XsText){.data = "ident", .length = 5}) ||
         xs_macro_text_equal(kind, (XsText){.data = "literal", .length = 7}) ||
         xs_macro_text_equal(kind, (XsText){.data = "lifetime", .length = 8}) ||
         xs_macro_text_equal(kind, (XsText){.data = "vis", .length = 3});
}

bool xs_macro_fragment_kind_is(const XsSyntaxNode *fragment, const char *kind)
{
  size_t length = strlen(kind);
  return fragment->child_count >= 2 && fragment->children[1]->text.length == length &&
         memcmp(fragment->children[1]->text.data, kind, length) == 0;
}

bool xs_macro_fragment_supported(XsText kind)
{
  return single_token_fragment_supported(kind) || xs_macro_text_equal(kind, (XsText){.data = "stmt", .length = 4}) ||
         xs_macro_text_equal(kind, (XsText){.data = "block", .length = 5});
}

bool xs_macro_fragment_matches(const XsSyntaxNode *fragment, const XsSyntaxNode *argument)
{
  if (fragment->child_count < 2)
    return false;
  XsText kind = fragment->children[1]->text;
  if (xs_macro_text_equal(kind, (XsText){.data = "tt", .length = 2}))
    return true;
  if (xs_macro_text_equal(kind, (XsText){.data = "ident", .length = 5}))
    return argument->token_kind == XS_TOKEN_IDENTIFIER;
  if (xs_macro_text_equal(kind, (XsText){.data = "literal", .length = 7}))
    return token_is_literal(argument->token_kind);
  if (xs_macro_text_equal(kind, (XsText){.data = "lifetime", .length = 8}))
    return argument->token_kind == XS_TOKEN_LIFETIME;
  if (xs_macro_text_equal(kind, (XsText){.data = "vis", .length = 3}))
    return token_is_visibility(argument->token_kind);
  return false;
}
