#include "xs/macro.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
  const XsSyntaxNode **items;
  size_t count;
  size_t capacity;
} MacroList;

static bool list_add(MacroList *list, const XsSyntaxNode *node)
{
  if (list->count == list->capacity) {
    size_t capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    const XsSyntaxNode **items = realloc(list->items, capacity * sizeof(*items));
    if (items == NULL)
      return false;
    list->items = items;
    list->capacity = capacity;
  }
  list->items[list->count++] = node;
  return true;
}

static bool text_equal(XsText left, XsText right)
{
  return left.length == right.length && memcmp(left.data, right.data, left.length) == 0;
}

static XsText macro_name(const XsSyntaxNode *macro)
{
  for (size_t i = 0; i < macro->child_count; ++i) {
    if (macro->children[i]->kind == XS_SYNTAX_IDENTIFIER)
      return macro->children[i]->text;
  }
  return (XsText){0};
}

static XsText call_name(const XsSyntaxNode *call)
{
  if (call->kind == XS_SYNTAX_STMT_MACRO_CALL) {
    for (size_t i = 0; i < call->child_count; ++i) {
      XsText name = call_name(call->children[i]);
      if (name.data != NULL)
        return name;
    }
    return (XsText){0};
  }
  if (call->kind != XS_SYNTAX_EXPR_MACRO_CALL)
    return (XsText){0};
  for (size_t i = 0; i < call->child_count; ++i) {
    if (call->children[i]->kind == XS_SYNTAX_IDENTIFIER)
      return call->children[i]->text;
  }
  return (XsText){0};
}

static bool token_text_matches(const XsSyntaxNode *matcher, const XsSyntaxNode *argument)
{
  if (matcher->token_kind != argument->token_kind)
    return false;
  if (matcher->text.length == 0 || argument->text.length == 0)
    return true;
  return text_equal(matcher->text, argument->text);
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
  return text_equal(kind, (XsText){.data = "tt", .length = 2}) ||
         text_equal(kind, (XsText){.data = "ident", .length = 5}) ||
         text_equal(kind, (XsText){.data = "literal", .length = 7}) ||
         text_equal(kind, (XsText){.data = "lifetime", .length = 8}) ||
         text_equal(kind, (XsText){.data = "vis", .length = 3});
}

static bool fragment_matches(const XsSyntaxNode *fragment, const XsSyntaxNode *argument)
{
  if (fragment->child_count < 2)
    return false;
  XsText kind = fragment->children[1]->text;
  if (text_equal(kind, (XsText){.data = "tt", .length = 2}))
    return true;
  if (text_equal(kind, (XsText){.data = "ident", .length = 5}))
    return argument->token_kind == XS_TOKEN_IDENTIFIER;
  if (text_equal(kind, (XsText){.data = "literal", .length = 7}))
    return token_is_literal(argument->token_kind);
  if (text_equal(kind, (XsText){.data = "lifetime", .length = 8}))
    return argument->token_kind == XS_TOKEN_LIFETIME;
  if (text_equal(kind, (XsText){.data = "vis", .length = 3}))
    return token_is_visibility(argument->token_kind);
  return false;
}

static bool rule_supported(const XsSyntaxNode *rule)
{
  if (rule->child_count == 0)
    return false;
  const XsSyntaxNode *matcher = rule->children[0];
  for (size_t i = 0; i < matcher->child_count; ++i) {
    const XsSyntaxNode *element = matcher->children[i];
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_REPETITION)
      return false;
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_FRAGMENT) {
      if (element->child_count < 2 || !single_token_fragment_supported(element->children[1]->text))
        return false;
    }
  }
  return true;
}

static bool rule_matches(const XsSyntaxNode *rule, const XsSyntaxNode *call)
{
  const XsSyntaxNode *matcher = rule->children[0];
  size_t argument = 1;
  for (size_t i = 0; i < matcher->child_count; ++i) {
    if (argument >= call->child_count)
      return false;
    const XsSyntaxNode *element = matcher->children[i];
    const XsSyntaxNode *value = call->children[argument++];
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_TOKEN && !token_text_matches(element, value))
      return false;
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_FRAGMENT && !fragment_matches(element, value))
      return false;
  }
  return argument == call->child_count;
}

static const XsSyntaxNode *resolve_macro(const MacroList *visible, XsText name)
{
  for (size_t i = visible->count; i > 0; --i) {
    if (text_equal(macro_name(visible->items[i - 1]), name))
      return visible->items[i - 1];
  }
  return NULL;
}

static bool prepare_node(const XsSyntaxNode *node, const MacroList *visible, XsDiagnostics *diagnostics,
                         XsMacroExpansionReport *report);

static bool prepare_scope(const XsSyntaxNode *scope, const MacroList *inherited, XsDiagnostics *diagnostics,
                          XsMacroExpansionReport *report)
{
  MacroList visible = {0};
  for (size_t i = 0; i < inherited->count; ++i) {
    if (!list_add(&visible, inherited->items[i]))
      return false;
  }
  for (size_t i = 0; i < scope->child_count; ++i) {
    if (scope->children[i]->kind == XS_SYNTAX_DECL_MACRO && !list_add(&visible, scope->children[i]))
      return false;
  }
  for (size_t i = 0; i < scope->child_count; ++i) {
    if (scope->children[i]->kind != XS_SYNTAX_DECL_MACRO &&
        !prepare_node(scope->children[i], &visible, diagnostics, report)) {
      free(visible.items);
      return false;
    }
  }
  free(visible.items);
  return true;
}

static bool is_scope(XsSyntaxKind kind)
{
  return kind == XS_SYNTAX_FILE || kind == XS_SYNTAX_STMT_BLOCK || kind == XS_SYNTAX_DECL_CLASS ||
         kind == XS_SYNTAX_DECL_INTERFACE;
}

static bool prepare_call(const XsSyntaxNode *call, const MacroList *visible, XsDiagnostics *diagnostics,
                         XsMacroExpansionReport *report)
{
  ++report->calls_seen;
  const XsSyntaxNode *macro = resolve_macro(visible, call_name(call));
  if (macro == NULL)
    return true;
  ++report->calls_resolved;
  for (size_t i = 0; i < macro->child_count; ++i) {
    const XsSyntaxNode *rule = macro->children[i];
    if (rule->kind != XS_SYNTAX_MACRO_RULE || rule->child_count == 0)
      continue;
    if (!rule_supported(rule)) {
      ++report->calls_deferred;
      XsSpan span = {.start = call->span.start_offset, .end = call->span.end_offset};
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_NOTE, span,
                         "macro call expansion is deferred until structural fragment capture is implemented");
      return true;
    }
    if (rule_matches(rule, call)) {
      ++report->calls_expandable;
      return true;
    }
  }
  return true;
}

static bool prepare_node(const XsSyntaxNode *node, const MacroList *visible, XsDiagnostics *diagnostics,
                         XsMacroExpansionReport *report)
{
  if (node == NULL)
    return true;
  if (is_scope(node->kind))
    return prepare_scope(node, visible, diagnostics, report);
  if (node->kind == XS_SYNTAX_EXPR_MACRO_CALL)
    return prepare_call(node, visible, diagnostics, report);
  for (size_t i = 0; i < node->child_count; ++i) {
    if (!prepare_node(node->children[i], visible, diagnostics, report))
      return false;
  }
  return true;
}

bool xs_macro_prepare_expansion(const XsSyntaxTree *tree, XsDiagnostics *diagnostics, XsMacroExpansionReport *report)
{
  if (tree == NULL || tree->root == NULL || diagnostics == NULL || report == NULL)
    return false;
  *report = (XsMacroExpansionReport){0};
  MacroList empty = {0};
  if (!prepare_scope(tree->root, &empty, diagnostics, report)) {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while preparing macro expansion");
    return false;
  }
  return !xs_diagnostics_has_error(diagnostics);
}
