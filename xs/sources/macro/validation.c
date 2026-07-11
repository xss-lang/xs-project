/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
  const XsSyntaxNode **items;
  size_t count;
  size_t capacity;
} NodeList;

static bool node_list_add(NodeList *list, const XsSyntaxNode *node)
{
  if (list->count == list->capacity)
  {
    size_t capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    const XsSyntaxNode **items = realloc(list->items, capacity * sizeof(*items));
    if (items == nullptr)
      return false;
    list->items = items;
    list->capacity = capacity;
  }
  list->items[list->count++] = node;
  return true;
}

static void collect_kind(const XsSyntaxNode *node, XsSyntaxKind kind, NodeList *nodes)
{
  if (node == nullptr)
    return;
  if (node->kind == kind)
    node_list_add(nodes, node);
  for (size_t i = 0; i < node->child_count; ++i)
    collect_kind(node->children[i], kind, nodes);
}

static XsText macro_name(const XsSyntaxNode *macro)
{
  for (size_t i = 0; i < macro->child_count; ++i)
  {
    if (macro->children[i]->kind == XS_SYNTAX_IDENTIFIER)
      return macro->children[i]->text;
  }
  return (XsText){0};
}

static bool matcher_variable_depth(const XsSyntaxNode *node, XsText name, size_t depth, size_t *result)
{
  if (node == nullptr)
    return false;
  if (node->kind == XS_SYNTAX_MACRO_MATCHER_FRAGMENT && node->child_count != 0 &&
      xs_macro_text_equal(node->children[0]->text, name))
  {
    *result = depth;
    return true;
  }
  size_t child_depth = depth + (node->kind == XS_SYNTAX_MACRO_MATCHER_REPETITION ? 1 : 0);
  for (size_t i = 0; i < node->child_count; ++i)
  {
    if (matcher_variable_depth(node->children[i], name, child_depth, result))
      return true;
  }
  return false;
}

static void validate_expansion_variables(const XsSyntaxNode *node, const XsSyntaxNode *matcher,
                                         XsDiagnostics *diagnostics, size_t depth, bool *contains_variable)
{
  if (node == nullptr)
    return;
  if (node->kind == XS_SYNTAX_MACRO_EXPANSION_VARIABLE && node->child_count != 0)
  {
    *contains_variable = true;
    size_t matcher_depth = 0;
    XsSpan span = {.start = node->span.start_offset, .end = node->span.end_offset};
    if (!matcher_variable_depth(matcher, node->children[0]->text, 0, &matcher_depth))
    {
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                         "macro expansion variable is not captured by this rule matcher");
    }
    else if (matcher_depth != depth)
    {
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                         "macro variable must keep its matcher repetition nesting depth during expansion");
    }
  }
  if (node->kind == XS_SYNTAX_MACRO_EXPANSION_REPETITION)
  {
    bool repetition_has_variable = false;
    for (size_t i = 0; i < node->child_count; ++i)
      validate_expansion_variables(node->children[i], matcher, diagnostics, depth + 1, &repetition_has_variable);
    if (!repetition_has_variable)
    {
      XsSpan span = {.start = node->span.start_offset, .end = node->span.end_offset};
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                         "macro expansion repetition must contain at least one captured variable");
    }
    *contains_variable = *contains_variable || repetition_has_variable;
    return;
  }
  for (size_t i = 0; i < node->child_count; ++i)
    validate_expansion_variables(node->children[i], matcher, diagnostics, depth, contains_variable);
}

static void validate_macro_rules(const XsSyntaxNode *macro, XsDiagnostics *diagnostics)
{
  for (size_t i = 0; i < macro->child_count; ++i)
  {
    const XsSyntaxNode *rule = macro->children[i];
    if (rule->kind != XS_SYNTAX_MACRO_RULE || rule->child_count < 2)
      continue;
    bool contains_variable = false;
    validate_expansion_variables(rule->children[1], rule->children[0], diagnostics, 0, &contains_variable);
  }
}

static bool expansion_calls_name(const XsSyntaxNode *node, XsText name)
{
  if (node == nullptr)
    return false;
  for (size_t i = 0; i + 1 < node->child_count; ++i)
  {
    const XsSyntaxNode *first = node->children[i];
    const XsSyntaxNode *second = node->children[i + 1];
    if (first->kind == XS_SYNTAX_MACRO_EXPANSION_TOKEN && first->token_kind == XS_TOKEN_IDENTIFIER &&
        xs_macro_text_equal(first->text, name) && second->kind == XS_SYNTAX_MACRO_EXPANSION_TOKEN &&
        second->token_kind == XS_TOKEN_BANG)
      return true;
  }
  for (size_t i = 0; i < node->child_count; ++i)
  {
    if (expansion_calls_name(node->children[i], name))
      return true;
  }
  return false;
}

static bool macro_reaches(const NodeList *macros, size_t from, size_t target, bool *visiting)
{
  if (visiting[from])
    return false;
  visiting[from] = true;
  for (size_t i = 0; i < macros->count; ++i)
  {
    if (!expansion_calls_name(macros->items[from], macro_name(macros->items[i])))
      continue;
    if (i == target || macro_reaches(macros, i, target, visiting))
    {
      visiting[from] = false;
      return true;
    }
  }
  visiting[from] = false;
  return false;
}

static void validate_recursion(const NodeList *macros, XsDiagnostics *diagnostics)
{
  if (macros->count == 0)
    return;
  bool *visiting = calloc(macros->count, sizeof(*visiting));
  if (visiting == nullptr)
  {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, (XsSpan){0, 0},
                       "compiler ran out of memory while validating macro call graph");
    return;
  }
  for (size_t i = 0; i < macros->count; ++i)
  {
    memset(visiting, 0, macros->count * sizeof(*visiting));
    if (macro_reaches(macros, i, i, visiting))
    {
      XsSpan span = {.start = macros->items[i]->span.start_offset, .end = macros->items[i]->span.end_offset};
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span, "direct or indirect macro recursion is not allowed");
    }
  }
  free(visiting);
}

static bool is_macro_scope(XsSyntaxKind kind)
{
  return kind == XS_SYNTAX_FILE || kind == XS_SYNTAX_STMT_BLOCK || kind == XS_SYNTAX_DECL_CLASS ||
         kind == XS_SYNTAX_DECL_INTERFACE;
}

static XsText macro_call_name(const XsSyntaxNode *call)
{
  if (call == nullptr)
    return (XsText){0};
  if (call->kind == XS_SYNTAX_STMT_MACRO_CALL)
  {
    for (size_t i = 0; i < call->child_count; ++i)
    {
      XsText name = macro_call_name(call->children[i]);
      if (name.data != nullptr)
        return name;
    }
    return (XsText){0};
  }
  if (call->kind != XS_SYNTAX_EXPR_MACRO_CALL)
    return (XsText){0};
  for (size_t i = 0; i < call->child_count; ++i)
  {
    if (call->children[i]->kind == XS_SYNTAX_IDENTIFIER)
      return call->children[i]->text;
  }
  return (XsText){0};
}

static const XsSyntaxNode *resolve_macro(const NodeList *visible, XsText name)
{
  for (size_t i = visible->count; i > 0; --i)
  {
    if (xs_macro_text_equal(macro_name(visible->items[i - 1]), name))
      return visible->items[i - 1];
  }
  return nullptr;
}

typedef enum
{
  MATCH_NO,
  MATCH_YES,
  MATCH_DEFERRED,
} MatchStatus;

static MatchStatus match_rule_arguments(const XsSyntaxNode *rule, const XsSyntaxNode *call)
{
  if (rule == nullptr || rule->kind != XS_SYNTAX_MACRO_RULE || rule->child_count == 0)
    return MATCH_NO;
  const XsSyntaxNode *matcher = rule->children[0];
  size_t argument_index = 1;
  for (size_t i = 0; i < matcher->child_count; ++i)
  {
    const XsSyntaxNode *element = matcher->children[i];
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_REPETITION)
      return MATCH_DEFERRED;
    if (argument_index >= call->child_count)
      return MATCH_NO;
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_TOKEN)
    {
      const XsSyntaxNode *argument = call->children[argument_index++];
      if (!xs_macro_token_text_matches(element, argument))
        return MATCH_NO;
      continue;
    }
    if (element->kind == XS_SYNTAX_MACRO_MATCHER_FRAGMENT)
    {
      if (element->child_count < 2)
        return MATCH_NO;
      if (xs_macro_fragment_is_sequence(element))
        return i + 1 == matcher->child_count && argument_index < call->child_count ? MATCH_YES : MATCH_NO;
      if (!xs_macro_fragment_supported(element->children[1]->text))
        return MATCH_DEFERRED;
      const XsSyntaxNode *argument = call->children[argument_index++];
      if (!xs_macro_fragment_matches(element, argument))
        return MATCH_NO;
      continue;
    }
    return MATCH_NO;
  }
  return argument_index == call->child_count ? MATCH_YES : MATCH_NO;
}

static MatchStatus macro_matches_call(const XsSyntaxNode *macro, const XsSyntaxNode *call)
{
  MatchStatus result = MATCH_NO;
  for (size_t i = 0; i < macro->child_count; ++i)
  {
    MatchStatus status = match_rule_arguments(macro->children[i], call);
    if (status == MATCH_YES)
      return MATCH_YES;
    if (status == MATCH_DEFERRED)
      result = MATCH_DEFERRED;
  }
  return result;
}

static void validate_scope_calls(const XsSyntaxTree *tree, const XsSyntaxNode *scope, const NodeList *inherited,
                                 XsDiagnostics *diagnostics);

static void validate_node_calls(const XsSyntaxTree *tree, const XsSyntaxNode *node, const NodeList *visible,
                                XsDiagnostics *diagnostics)
{
  if (node == nullptr)
    return;
  if (is_macro_scope(node->kind))
  {
    validate_scope_calls(tree, node, visible, diagnostics);
    return;
  }
  if (node->kind == XS_SYNTAX_EXPR_MACRO_CALL)
  {
    XsText name = macro_call_name(node);
    const XsSyntaxNode *macro = resolve_macro(visible, name);
    XsSpan span = {.start = node->span.start_offset, .end = node->span.end_offset};
    if (macro == nullptr && !xs_macro_external_import_resolves(tree, name))
    {
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span, "macro call does not resolve in this scope");
      return;
    }
    if (macro != nullptr && macro_matches_call(macro, node) == MATCH_NO)
      xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span, "macro call does not match any rule");
    return;
  }
  for (size_t i = 0; i < node->child_count; ++i)
    validate_node_calls(tree, node->children[i], visible, diagnostics);
}

static void validate_scope_calls(const XsSyntaxTree *tree, const XsSyntaxNode *scope, const NodeList *inherited,
                                 XsDiagnostics *diagnostics)
{
  NodeList visible = {0};
  for (size_t i = 0; i < inherited->count; ++i)
    node_list_add(&visible, inherited->items[i]);
  for (size_t i = 0; i < scope->child_count; ++i)
  {
    if (scope->children[i]->kind == XS_SYNTAX_DECL_MACRO)
      node_list_add(&visible, scope->children[i]);
  }
  for (size_t i = 0; i < scope->child_count; ++i)
  {
    if (scope->children[i]->kind != XS_SYNTAX_DECL_MACRO)
      validate_node_calls(tree, scope->children[i], &visible, diagnostics);
  }
  free(visible.items);
}

static void validate_scope_recursion(const XsSyntaxNode *scope, XsDiagnostics *diagnostics)
{
  NodeList macros = {0};
  for (size_t i = 0; i < scope->child_count; ++i)
  {
    if (scope->children[i]->kind == XS_SYNTAX_DECL_MACRO)
      node_list_add(&macros, scope->children[i]);
  }
  validate_recursion(&macros, diagnostics);
  free(macros.items);
  for (size_t i = 0; i < scope->child_count; ++i)
  {
    if (is_macro_scope(scope->children[i]->kind))
      validate_scope_recursion(scope->children[i], diagnostics);
    else
    {
      for (size_t j = 0; j < scope->children[i]->child_count; ++j)
      {
        if (is_macro_scope(scope->children[i]->children[j]->kind))
          validate_scope_recursion(scope->children[i]->children[j], diagnostics);
      }
    }
  }
}

bool xs_macro_validate(const XsSyntaxTree *tree, XsDiagnostics *diagnostics)
{
  if (tree == nullptr || tree->root == nullptr || diagnostics == nullptr)
    return false;
  NodeList macros = {0};
  collect_kind(tree->root, XS_SYNTAX_DECL_MACRO, &macros);
  for (size_t i = 0; i < macros.count; ++i)
    validate_macro_rules(macros.items[i], diagnostics);
  validate_scope_recursion(tree->root, diagnostics);
  NodeList empty = {0};
  validate_scope_calls(tree, tree->root, &empty, diagnostics);
  free(macros.items);
  return !xs_diagnostics_has_error(diagnostics);
}
