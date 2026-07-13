/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "source_native.h"

#include "direct_xlil.h"
#include "source_native_internal.h"

#include "xs/mir/borrow_checker.h"
#include "xs/mir/optimizer.h"
#include "xs/mir/xlil_lowering.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define node_span xs_source_native_node_span
#define text_equals xs_source_native_text_equals
#define node_text_equals xs_source_native_node_text_equals
#define copy_node_text xs_source_native_copy_node_text
#define copy_cstr xs_source_native_copy_cstr
#define set_mir_error xs_source_native_set_mir_error
#define context_init_parameters xs_source_native_context_init_parameters
#define context_add_local xs_source_native_context_add_local
#define context_read xs_source_native_context_read
#define context_type xs_source_native_context_type
#define context_assign xs_source_native_context_assign
#define first_child_kind xs_source_native_first_child_kind
#define child_count_kind xs_source_native_child_count_kind
#define parse_i32_literal xs_source_native_parse_i32_literal

static bool lower_i32_expression(XsMirBlock *entry, const NativeContext *context, const NativeProgram *program,
                                 XsText current_function, const XsSyntaxNode *expression, XsDiagnostics *diagnostics,
                                 XsMirValueId *result, XsMirError *error);

static bool lower_bool_expression(XsMirBlock *entry, const NativeContext *context, const NativeProgram *program,
                                  XsText current_function, const XsSyntaxNode *expression, XsDiagnostics *diagnostics,
                                  XsMirValueId *result, bool *invert, XsMirError *error);

static bool lower_call_expression_with_type(XsMirBlock *entry, const NativeContext *context,
                                            const NativeProgram *program, XsText current_function,
                                            const XsSyntaxNode *expression, XsLilTypeKind expected,
                                            XsDiagnostics *diagnostics, XsMirValueId *result, XsMirError *error);

static const XsSyntaxNode *single_block_expression(const XsSyntaxNode *block)
{
  if(block == nullptr || block->kind != XS_SYNTAX_STMT_BLOCK || block->child_count != 1)
    return nullptr;
  const XsSyntaxNode *statement = block->children[0];
  if(statement->kind != XS_SYNTAX_STMT_EXPRESSION || statement->child_count != 1)
    return nullptr;
  return statement->children[0];
}

static bool lower_bool_expression(XsMirBlock *entry, const NativeContext *context, const NativeProgram *program,
                                  XsText current_function, const XsSyntaxNode *expression, XsDiagnostics *diagnostics,
                                  XsMirValueId *result, bool *invert, XsMirError *error)
{
  *invert = false;
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_BANG &&
     expression->child_count == 1)
  {
    bool nested_invert = false;
    if(!lower_bool_expression(entry, context, program, current_function, expression->children[0], diagnostics, result,
                              &nested_invert, error))
      return false;
    *invert = !nested_invert;
    return true;
  }
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL &&
     (expression->token_kind == XS_TOKEN_KW_TRUE || expression->token_kind == XS_TOKEN_KW_FALSE))
    return xs_mir_block_add_const_bool(entry, expression->token_kind == XS_TOKEN_KW_TRUE, result, error) == XS_MIR_OK;
  if(expression->kind == XS_SYNTAX_EXPR_IDENTIFIER)
  {
    if(context_read(context, entry, expression->text, XS_LIL_TYPE_BOOL, result, error))
      return true;
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                              "native source main condition identifier does not resolve to a supported Bool local") &&
           false;
  }
  if(expression->kind == XS_SYNTAX_EXPR_BINARY && expression->child_count == 3)
  {
    XsMirValueId left = 0;
    XsMirValueId right = 0;
    if(!lower_i32_expression(entry, context, program, current_function, expression->children[0], diagnostics, &left,
                             error) ||
       !lower_i32_expression(entry, context, program, current_function, expression->children[2], diagnostics, &right,
                             error))
      return false;
    switch(expression->token_kind)
    {
    case XS_TOKEN_EQUAL:
      return xs_mir_block_eq_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_NOT_EQUAL:
      return xs_mir_block_ne_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_LESS:
      return xs_mir_block_lt_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_LESS_EQUAL:
      return xs_mir_block_le_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_GREATER:
      return xs_mir_block_gt_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_GREATER_EQUAL:
      return xs_mir_block_ge_i32(entry, left, right, result, error) == XS_MIR_OK;
    default:
      break;
    }
  }
  if(expression->kind == XS_SYNTAX_EXPR_CALL)
  {
    XsMirValueId value = 0;
    if(!lower_call_expression_with_type(entry, context, program, current_function, expression, XS_LIL_TYPE_BOOL,
                                        diagnostics, &value, error))
      return false;
    *result = value;
    return true;
  }
  return xs_diagnostics_add(
             diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
             "native source main if condition supports only !, bool literals, and i32 comparisons for now") &&
         false;
}

static bool static_bool_condition(const XsSyntaxNode *expression, bool *value)
{
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL &&
     (expression->token_kind == XS_TOKEN_KW_TRUE || expression->token_kind == XS_TOKEN_KW_FALSE))
  {
    *value = expression->token_kind == XS_TOKEN_KW_TRUE;
    return true;
  }
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_BANG &&
     expression->child_count == 1 && static_bool_condition(expression->children[0], value))
  {
    *value = !*value;
    return true;
  }
  return false;
}

static bool static_i32_expression(const XsSyntaxNode *expression, int32_t *value)
{
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_MINUS &&
     expression->child_count == 1)
  {
    int32_t nested = 0;
    if(!static_i32_expression(expression->children[0], &nested))
      return false;
    if(nested == INT32_MIN)
      return false;
    *value = -nested;
    return true;
  }
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_PLUS &&
     expression->child_count == 1)
    return static_i32_expression(expression->children[0], value);
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL && expression->token_kind == XS_TOKEN_INTEGER)
    return parse_i32_literal(expression, value);
  if(expression->kind != XS_SYNTAX_EXPR_BINARY || expression->child_count != 3)
    return false;
  int32_t left = 0;
  int32_t right = 0;
  if(!static_i32_expression(expression->children[0], &left) || !static_i32_expression(expression->children[2], &right))
    return false;
  if((expression->token_kind == XS_TOKEN_SLASH || expression->token_kind == XS_TOKEN_PERCENT) &&
     (right == 0 || (left == INT32_MIN && right == -1)))
    return false;
  if((expression->token_kind == XS_TOKEN_SHIFT_LEFT || expression->token_kind == XS_TOKEN_SHIFT_RIGHT) &&
     (right < 0 || right >= 32))
    return false;
  if(expression->token_kind == XS_TOKEN_SHIFT_LEFT && (left < 0 || left > (INT32_MAX >> (unsigned)right)))
    return false;
  if(expression->token_kind == XS_TOKEN_SHIFT_RIGHT && left < 0)
    return false;
  int64_t result = expression->token_kind == XS_TOKEN_PLUS          ? (int64_t)left + right
                   : expression->token_kind == XS_TOKEN_MINUS       ? (int64_t)left - right
                   : expression->token_kind == XS_TOKEN_STAR        ? (int64_t)left * right
                   : expression->token_kind == XS_TOKEN_SLASH       ? left / right
                   : expression->token_kind == XS_TOKEN_PERCENT     ? left % right
                   : expression->token_kind == XS_TOKEN_AMPERSAND   ? left & right
                   : expression->token_kind == XS_TOKEN_PIPE        ? left | right
                   : expression->token_kind == XS_TOKEN_CARET       ? left ^ right
                   : expression->token_kind == XS_TOKEN_SHIFT_LEFT  ? left << (unsigned)right
                   : expression->token_kind == XS_TOKEN_SHIFT_RIGHT ? left >> (unsigned)right
                                                                    : (int64_t)INT32_MIN - 1;
  if(result < INT32_MIN || result > INT32_MAX)
    return false;
  *value = (int32_t)result;
  return true;
}

static bool static_i32_comparison(const XsSyntaxNode *expression, bool *value)
{
  if(expression->kind != XS_SYNTAX_EXPR_BINARY || expression->child_count != 3)
    return false;
  int32_t left = 0;
  int32_t right = 0;
  if(!static_i32_expression(expression->children[0], &left) || !static_i32_expression(expression->children[2], &right))
    return false;
  switch(expression->token_kind)
  {
  case XS_TOKEN_EQUAL:
    *value = left == right;
    return true;
  case XS_TOKEN_NOT_EQUAL:
    *value = left != right;
    return true;
  case XS_TOKEN_LESS:
    *value = left < right;
    return true;
  case XS_TOKEN_LESS_EQUAL:
    *value = left <= right;
    return true;
  case XS_TOKEN_GREATER:
    *value = left > right;
    return true;
  case XS_TOKEN_GREATER_EQUAL:
    *value = left >= right;
    return true;
  default:
    return false;
  }
}

static bool static_source_condition(const XsSyntaxNode *expression, bool *value)
{
  if(static_bool_condition(expression, value) || static_i32_comparison(expression, value))
    return true;
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_BANG &&
     expression->child_count == 1 && static_source_condition(expression->children[0], value))
  {
    *value = !*value;
    return true;
  }
  return false;
}

static bool is_bool_expression_shape(const XsSyntaxNode *expression)
{
  if(expression == nullptr)
    return false;
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL &&
     (expression->token_kind == XS_TOKEN_KW_TRUE || expression->token_kind == XS_TOKEN_KW_FALSE))
    return true;
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_BANG &&
     expression->child_count == 1)
    return true;
  if(expression->kind != XS_SYNTAX_EXPR_BINARY || expression->child_count != 3)
    return false;
  return expression->token_kind == XS_TOKEN_EQUAL || expression->token_kind == XS_TOKEN_NOT_EQUAL ||
         expression->token_kind == XS_TOKEN_LESS || expression->token_kind == XS_TOKEN_LESS_EQUAL ||
         expression->token_kind == XS_TOKEN_GREATER || expression->token_kind == XS_TOKEN_GREATER_EQUAL;
}

static const XsSyntaxNode *simple_call_name(const XsSyntaxNode *expression)
{
  if(expression == nullptr || expression->kind != XS_SYNTAX_EXPR_CALL || expression->child_count == 0)
    return nullptr;
  const XsSyntaxNode *callee = expression->children[0];
  if(callee->kind != XS_SYNTAX_EXPR_IDENTIFIER || callee->child_count != 1 ||
     callee->children[0]->kind != XS_SYNTAX_IDENTIFIER)
    return nullptr;
  return callee->children[0];
}

static const NativeFunction *resolve_call_target(const NativeProgram *program, XsText current_function,
                                                 const XsSyntaxNode *expression, XsDiagnostics *diagnostics)
{
  const XsSyntaxNode *callee_name = simple_call_name(expression);
  if(callee_name == nullptr)
  {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                       "native source direct calls must target a same-module function name");
    return nullptr;
  }
  if(text_equals(callee_name->text, current_function))
  {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(callee_name),
                       "native source direct calls do not support recursion in this compiler slice");
    return nullptr;
  }
  const NativeFunction *callee = xs_source_native_program_find_function(program, callee_name->text);
  if(callee == nullptr)
  {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(callee_name),
                       "native source direct call target is unknown in this compiler slice");
    return nullptr;
  }
  size_t argument_count = expression->child_count - 1U;
  if(argument_count != callee->parameter_count)
  {
    xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                       "native source direct call argument count does not match the callee");
    return nullptr;
  }
  return callee;
}

static bool lower_call_expression_with_type(XsMirBlock *entry, const NativeContext *context,
                                            const NativeProgram *program, XsText current_function,
                                            const XsSyntaxNode *expression, XsLilTypeKind expected,
                                            XsDiagnostics *diagnostics, XsMirValueId *result, XsMirError *error)
{
  const NativeFunction *callee = resolve_call_target(program, current_function, expression, diagnostics);
  if(callee == nullptr)
    return false;
  if(callee->return_kind != expected)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                              expected == XS_LIL_TYPE_I32
                                  ? "native source direct call must return Long in this expression"
                                  : "native source direct call must return Bool in this expression") &&
           false;
  size_t argument_count = expression->child_count - 1U;
  XsMirValueId arguments[16] = {0};
  if(argument_count > sizeof(arguments) / sizeof(arguments[0]))
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                              "native source direct calls support at most 16 arguments for now") &&
           false;
  for(size_t i = 0; i < argument_count; ++i)
  {
    if(!lower_i32_expression(entry, context, program, current_function, expression->children[i + 1U], diagnostics,
                             &arguments[i], error))
      return false;
  }
  const XsSyntaxNode *callee_name = simple_call_name(expression);
  char *callee_text = copy_node_text(callee_name);
  if(callee_text == nullptr)
  {
    set_mir_error(error, XS_MIR_ALLOCATION_FAILED, "out of memory while copying native source callee");
    return false;
  }
  bool ok = xs_mir_block_add_call(entry, callee_text, (XsMirType){.kind = expected}, arguments, argument_count, result,
                                  error) == XS_MIR_OK;
  free(callee_text);
  return ok;
}

static bool lower_i32_expression(XsMirBlock *entry, const NativeContext *context, const NativeProgram *program,
                                 XsText current_function, const XsSyntaxNode *expression, XsDiagnostics *diagnostics,
                                 XsMirValueId *result, XsMirError *error)
{
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_MINUS &&
     expression->child_count == 1)
  {
    XsMirValueId zero = 0;
    XsMirValueId value = 0;
    return xs_mir_block_add_const_i32(entry, 0, &zero, error) == XS_MIR_OK &&
           lower_i32_expression(entry, context, program, current_function, expression->children[0], diagnostics, &value,
                                error) &&
           xs_mir_block_sub_i32(entry, zero, value, result, error) == XS_MIR_OK;
  }
  if(expression->kind == XS_SYNTAX_EXPR_UNARY && expression->token_kind == XS_TOKEN_PLUS &&
     expression->child_count == 1)
    return lower_i32_expression(entry, context, program, current_function, expression->children[0], diagnostics, result,
                                error);
  if(expression->kind == XS_SYNTAX_EXPR_IDENTIFIER)
  {
    if(context_read(context, entry, expression->text, XS_LIL_TYPE_I32, result, error))
      return true;
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                              "native source main identifier does not resolve to a supported Long local binding") &&
           false;
  }
  if(expression->kind == XS_SYNTAX_EXPR_LITERAL && expression->token_kind == XS_TOKEN_INTEGER)
  {
    int32_t value = 0;
    if(!parse_i32_literal(expression, &value))
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                                "native source main integer literal must fit in i32") &&
             false;
    return xs_mir_block_add_const_i32(entry, value, result, error) == XS_MIR_OK;
  }
  if(expression->kind == XS_SYNTAX_EXPR_CALL)
    return lower_call_expression_with_type(entry, context, program, current_function, expression, XS_LIL_TYPE_I32,
                                           diagnostics, result, error);
  if(expression->kind == XS_SYNTAX_EXPR_BINARY && expression->child_count == 3)
  {
    XsMirValueId left = 0;
    XsMirValueId right = 0;
    if(!lower_i32_expression(entry, context, program, current_function, expression->children[0], diagnostics, &left,
                             error) ||
       !lower_i32_expression(entry, context, program, current_function, expression->children[2], diagnostics, &right,
                             error))
      return false;
    switch(expression->token_kind)
    {
    case XS_TOKEN_PLUS:
      return xs_mir_block_add_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_MINUS:
      return xs_mir_block_sub_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_STAR:
      return xs_mir_block_mul_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_SLASH:
      return xs_mir_block_div_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_PERCENT:
      return xs_mir_block_rem_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_AMPERSAND:
      return xs_mir_block_and_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_PIPE:
      return xs_mir_block_or_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_CARET:
      return xs_mir_block_xor_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_SHIFT_LEFT:
      return xs_mir_block_shl_i32(entry, left, right, result, error) == XS_MIR_OK;
    case XS_TOKEN_SHIFT_RIGHT:
      return xs_mir_block_shr_i32(entry, left, right, result, error) == XS_MIR_OK;
    default:
      break;
    }
  }
  return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                            "native source main return expression supports only integer literals, unary +/-, +, -, *, "
                            "/, %, &, |, ^, <<, >>, direct Long calls, and top-level if for now") &&
         false;
}

static bool lower_if_return(XsMirFunction *function, XsMirBlock *entry, const NativeContext *context,
                            const NativeProgram *program, XsText current_function, const XsSyntaxNode *expression,
                            XsDiagnostics *diagnostics, XsMirError *error)
{
  if(expression->kind != XS_SYNTAX_EXPR_IF || expression->child_count < 3)
    return false;
  const XsSyntaxNode *then_expression = single_block_expression(expression->children[1]);
  const XsSyntaxNode *else_expression = single_block_expression(expression->children[2]);
  if(then_expression == nullptr || else_expression == nullptr)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(expression),
                              "native source main if branches must each contain exactly one expression statement") &&
           false;
  bool static_condition = false;
  if(static_source_condition(expression->children[0], &static_condition))
  {
    XsMirValueId selected_value = 0;
    const XsSyntaxNode *selected = static_condition ? then_expression : else_expression;
    return lower_i32_expression(entry, context, program, current_function, selected, diagnostics, &selected_value,
                                error) &&
           xs_mir_block_set_return_value(entry, selected_value, error) == XS_MIR_OK;
  }
  XsMirBlock *then_block = nullptr;
  XsMirBlock *else_block = nullptr;
  XsMirValueId condition = 0;
  bool invert_condition = false;
  if(!lower_bool_expression(entry, context, program, current_function, expression->children[0], diagnostics, &condition,
                            &invert_condition, error))
    return false;
  if(xs_mir_function_append_block(function, "then", &then_block, error) != XS_MIR_OK ||
     xs_mir_function_append_block(function, "else", &else_block, error) != XS_MIR_OK ||
     xs_mir_block_set_branch(entry, condition, invert_condition ? else_block : then_block,
                             invert_condition ? then_block : else_block, error) != XS_MIR_OK)
    return false;
  XsMirValueId then_value = 0;
  XsMirValueId else_value = 0;
  return lower_i32_expression(then_block, context, program, current_function, then_expression, diagnostics, &then_value,
                              error) &&
         xs_mir_block_set_return_value(then_block, then_value, error) == XS_MIR_OK &&
         lower_i32_expression(else_block, context, program, current_function, else_expression, diagnostics, &else_value,
                              error) &&
         xs_mir_block_set_return_value(else_block, else_value, error) == XS_MIR_OK;
}

static const XsSyntaxNode *variable_initializer(const XsSyntaxNode *declaration)
{
  if(declaration == nullptr || declaration->kind != XS_SYNTAX_DECL_VARIABLE)
    return nullptr;
  if((declaration->flags & XS_SYNTAX_FLAG_INFERRED_TYPE) != 0)
    return declaration->child_count >= 2 ? declaration->children[1] : nullptr;
  return declaration->child_count >= 3 ? declaration->children[2] : nullptr;
}

static bool lower_local_statement(XsMirFunction *function, XsMirBlock *entry, NativeContext *context,
                                  const NativeProgram *program, XsText current_function, const XsSyntaxNode *statement,
                                  XsDiagnostics *diagnostics, XsMirError *error)
{
  const XsSyntaxNode *declaration = statement->child_count == 1 ? statement->children[0] : nullptr;
  const XsSyntaxNode *name =
      declaration != nullptr && declaration->child_count != 0 && declaration->children[0]->kind == XS_SYNTAX_IDENTIFIER
          ? declaration->children[0]
          : nullptr;
  const XsSyntaxNode *type = declaration != nullptr && (declaration->flags & XS_SYNTAX_FLAG_INFERRED_TYPE) == 0 &&
                                     declaration->child_count >= 2
                                 ? declaration->children[1]
                                 : nullptr;
  const XsSyntaxNode *initializer = variable_initializer(declaration);
  bool inferred = declaration != nullptr && (declaration->flags & XS_SYNTAX_FLAG_INFERRED_TYPE) != 0;
  if(name == nullptr || initializer == nullptr ||
     (!inferred && !node_text_equals(type, "Long") && !node_text_equals(type, "Bool")))
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(statement),
                              "native source local declarations support only Long, Bool, or inferred bindings "
                              "with initializers for now") &&
           false;
  XsMirValueId value = 0;
  if(node_text_equals(type, "Bool") || (inferred && is_bool_expression_shape(initializer)))
  {
    bool invert = false;
    if(!lower_bool_expression(entry, context, program, current_function, initializer, diagnostics, &value, &invert,
                              error))
      return false;
    if(invert)
    {
      XsMirValueId inverted = 0;
      if(xs_mir_block_not_bool(entry, value, &inverted, error) != XS_MIR_OK)
        return false;
      value = inverted;
    }
    bool is_mutable = (declaration->flags &
                       (XS_SYNTAX_FLAG_IMMUTABLE | XS_SYNTAX_FLAG_CONSTANT | XS_SYNTAX_FLAG_STATIC_CONSTANT)) == 0;
    return context_add_local(context, function, entry, name->text, value, XS_LIL_TYPE_BOOL, is_mutable, diagnostics,
                             node_span(name), error);
  }
  if(!lower_i32_expression(entry, context, program, current_function, initializer, diagnostics, &value, error))
    return false;
  bool is_mutable =
      (declaration->flags & (XS_SYNTAX_FLAG_IMMUTABLE | XS_SYNTAX_FLAG_CONSTANT | XS_SYNTAX_FLAG_STATIC_CONSTANT)) == 0;
  return context_add_local(context, function, entry, name->text, value, XS_LIL_TYPE_I32, is_mutable, diagnostics,
                           node_span(name), error);
}

static bool lower_assignment_statement(XsMirBlock *entry, const NativeContext *context, const NativeProgram *program,
                                       XsText current_function, const XsSyntaxNode *statement,
                                       XsDiagnostics *diagnostics, XsMirError *error)
{
  const XsSyntaxNode *assignment =
      statement->kind == XS_SYNTAX_STMT_EXPRESSION && statement->child_count == 1 ? statement->children[0] : nullptr;
  if(assignment == nullptr || assignment->kind != XS_SYNTAX_EXPR_ASSIGNMENT || assignment->child_count != 3 ||
     assignment->children[0]->kind != XS_SYNTAX_EXPR_IDENTIFIER)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(statement),
                              "native source statements before return must be local declarations or simple "
                              "assignments") &&
           false;
  const XsSyntaxNode *target = assignment->children[0];
  const XsSyntaxNode *value_expression = assignment->children[2];
  XsLilTypeKind type = XS_LIL_TYPE_VOID;
  if(!context_type(context, target->text, &type))
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(target),
                              "native source assignment target is unknown in this compiler slice") &&
           false;
  XsMirValueId value = 0;
  if(type == XS_LIL_TYPE_BOOL)
  {
    if(assignment->token_kind != XS_TOKEN_ASSIGN)
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(assignment),
                                "native source compound assignment requires a Long local") &&
             false;
    bool invert = false;
    if(!lower_bool_expression(entry, context, program, current_function, value_expression, diagnostics, &value, &invert,
                              error))
      return false;
    if(invert)
    {
      XsMirValueId inverted = 0;
      if(xs_mir_block_not_bool(entry, value, &inverted, error) != XS_MIR_OK)
        return false;
      value = inverted;
    }
  }
  else if(type == XS_LIL_TYPE_I32)
  {
    if(!lower_i32_expression(entry, context, program, current_function, value_expression, diagnostics, &value, error))
      return false;
    if(assignment->token_kind != XS_TOKEN_ASSIGN)
    {
      XsMirValueId current = 0;
      if(!context_read(context, entry, target->text, XS_LIL_TYPE_I32, &current, error))
        return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(target),
                                  "native source compound assignment target is not a supported Long local") &&
               false;
      switch(assignment->token_kind)
      {
      case XS_TOKEN_PLUS_ASSIGN:
        if(xs_mir_block_add_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_MINUS_ASSIGN:
        if(xs_mir_block_sub_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_STAR_ASSIGN:
        if(xs_mir_block_mul_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_SLASH_ASSIGN:
        if(xs_mir_block_div_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_PERCENT_ASSIGN:
        if(xs_mir_block_rem_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_AMPERSAND_ASSIGN:
        if(xs_mir_block_and_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_PIPE_ASSIGN:
        if(xs_mir_block_or_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      case XS_TOKEN_CARET_ASSIGN:
        if(xs_mir_block_xor_i32(entry, current, value, &value, error) != XS_MIR_OK)
          return false;
        break;
      default:
        return xs_diagnostics_add(
                   diagnostics, XS_DIAGNOSTIC_ERROR, node_span(assignment),
                   "native source compound assignment operator is not supported in this compiler slice") &&
               false;
      }
    }
  }
  else
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(target),
                              "native source assignment supports only Long and Bool locals for now") &&
           false;
  return context_assign(context, entry, target->text, type, value, diagnostics, node_span(target), error);
}

static bool lower_if_statement(XsMirFunction *function, XsMirBlock **current, const NativeContext *context,
                               const NativeProgram *program, XsText current_function, const XsSyntaxNode *statement,
                               XsDiagnostics *diagnostics, XsMirError *error);

static bool lower_statement_block(XsMirFunction *function, XsMirBlock **current, const NativeContext *context,
                                  const NativeProgram *program, XsText current_function, const XsSyntaxNode *block,
                                  XsDiagnostics *diagnostics, XsMirError *error)
{
  if(block == nullptr || block->kind != XS_SYNTAX_STMT_BLOCK || block->child_count == 0)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(block),
                              "native source if branches must contain one or more supported statements for now") &&
           false;
  for(size_t index = 0; index < block->child_count; ++index)
  {
    const XsSyntaxNode *statement = block->children[index];
    bool lowered =
        statement->kind == XS_SYNTAX_STMT_IF
            ? lower_if_statement(function, current, context, program, current_function, statement, diagnostics, error)
            : lower_assignment_statement(*current, context, program, current_function, statement, diagnostics, error);
    if(!lowered)
      return false;
  }
  return true;
}

static bool lower_if_statement(XsMirFunction *function, XsMirBlock **current, const NativeContext *context,
                               const NativeProgram *program, XsText current_function, const XsSyntaxNode *statement,
                               XsDiagnostics *diagnostics, XsMirError *error)
{
  if(statement->child_count < 2 || statement->child_count > 3)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, node_span(statement),
                              "native source if statements do not support else if in this compiler slice") &&
           false;
  XsMirValueId condition = 0;
  bool invert = false;
  if(!lower_bool_expression(*current, context, program, current_function, statement->children[0], diagnostics,
                            &condition, &invert, error))
    return false;
  XsMirBlock *then_block = nullptr;
  XsMirBlock *else_block = nullptr;
  XsMirBlock *merge_block = nullptr;
  if(xs_mir_function_append_block(function, "then", &then_block, error) != XS_MIR_OK ||
     xs_mir_function_append_block(function, "else", &else_block, error) != XS_MIR_OK ||
     xs_mir_function_append_block(function, "merge", &merge_block, error) != XS_MIR_OK ||
     xs_mir_block_set_branch(*current, condition, invert ? else_block : then_block, invert ? then_block : else_block,
                             error) != XS_MIR_OK)
    return false;
  XsMirBlock *then_current = then_block;
  XsMirBlock *else_current = else_block;
  if(!lower_statement_block(function, &then_current, context, program, current_function, statement->children[1],
                            diagnostics, error) ||
     xs_mir_block_set_goto(then_current, merge_block, error) != XS_MIR_OK)
    return false;
  if(statement->child_count == 3 && !lower_statement_block(function, &else_current, context, program, current_function,
                                                           statement->children[2], diagnostics, error))
    return false;
  if(xs_mir_block_set_goto(else_current, merge_block, error) != XS_MIR_OK)
    return false;
  *current = merge_block;
  return true;
}

bool xs_source_native_lower_function_body(XsMirFunction *function, XsMirBlock *entry, const NativeFunction *native,
                                          const NativeProgram *program, XsDiagnostics *diagnostics, XsMirError *error)
{
  NativeContext context = {0};
  context_init_parameters(&context, native->function);
  XsMirBlock *current = entry;
  for(size_t i = 0; i + 1 < native->body->child_count; ++i)
  {
    const XsSyntaxNode *statement = native->body->children[i];
    bool lowered =
        statement->kind == XS_SYNTAX_STMT_VARIABLE
            ? lower_local_statement(function, current, &context, program, native->name->text, statement, diagnostics,
                                    error)
        : statement->kind == XS_SYNTAX_STMT_IF
            ? lower_if_statement(function, &current, &context, program, native->name->text, statement, diagnostics,
                                 error)
            : lower_assignment_statement(current, &context, program, native->name->text, statement, diagnostics, error);
    if(!lowered)
      return false;
  }
  const XsSyntaxNode *statement = native->body->children[native->body->child_count - 1];
  const XsSyntaxNode *expression = statement->children[0];
  if(expression->kind == XS_SYNTAX_EXPR_IF)
    return lower_if_return(function, current, &context, program, native->name->text, expression, diagnostics, error);
  if(native->return_kind == XS_LIL_TYPE_BOOL)
  {
    XsMirValueId value = 0;
    bool invert = false;
    if(!lower_bool_expression(current, &context, program, native->name->text, expression, diagnostics, &value, &invert,
                              error))
      return false;
    if(invert)
    {
      XsMirValueId inverted = 0;
      if(xs_mir_block_not_bool(current, value, &inverted, error) != XS_MIR_OK)
        return false;
      value = inverted;
    }
    return xs_mir_block_set_return_value(current, value, error) == XS_MIR_OK;
  }
  XsMirValueId value = 0;
  return lower_i32_expression(current, &context, program, native->name->text, expression, diagnostics, &value, error) &&
         xs_mir_block_set_return_value(current, value, error) == XS_MIR_OK;
}
