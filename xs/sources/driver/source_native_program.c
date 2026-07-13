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

static const XsSyntaxNode *return_type(const XsSyntaxNode *function)
{
  for(size_t index = 0; index < function->child_count; ++index)
  {
    const XsSyntaxNode *child = function->children[index];
    if((child->flags & XS_SYNTAX_FLAG_RETURN_TYPE) != 0)
      return child;
  }
  return nullptr;
}

const NativeFunction *xs_source_native_program_find_function(const NativeProgram *program, XsText name)
{
  for(size_t index = 0; index < program->count; ++index)
  {
    if(xs_source_native_text_equals(program->functions[index].name->text, name))
      return &program->functions[index];
  }
  return nullptr;
}

static bool program_has_name(const NativeProgram *program, XsText name)
{
  return xs_source_native_program_find_function(program, name) != nullptr;
}

static bool validate_shape(const XsSyntaxNode *function, bool is_main, XsDiagnostics *diagnostics,
                           const XsSyntaxNode **body, size_t *parameter_count, XsLilTypeKind *return_kind)
{
  *parameter_count = xs_source_native_child_count_kind(function, XS_SYNTAX_PARAMETER);
  if(is_main && *parameter_count != 0)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(function),
                              "native source main must not have parameters") &&
           false;
  if(xs_source_native_child_count_kind(function, XS_SYNTAX_GENERIC_PARAMETER) != 0)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(function),
                              "native source main must not be generic") &&
           false;
  const XsSyntaxNode *type = return_type(function);
  if(xs_source_native_node_text_equals(type, "Long"))
    *return_kind = XS_LIL_TYPE_I32;
  else if(!is_main && xs_source_native_node_text_equals(type, "Bool"))
    *return_kind = XS_LIL_TYPE_BOOL;
  else
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(function),
                              is_main
                                  ? "native source main must return Long in this compiler slice"
                                  : "native source helper functions must return Long or Bool in this compiler slice") &&
           false;
  if(!is_main)
  {
    for(size_t index = 0; index < function->child_count; ++index)
    {
      const XsSyntaxNode *parameter = function->children[index];
      if(parameter->kind != XS_SYNTAX_PARAMETER)
        continue;
      const XsSyntaxNode *parameter_type = parameter->child_count >= 2 ? parameter->children[1] : nullptr;
      if(!xs_source_native_node_text_equals(parameter_type, "Long"))
        return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(parameter),
                                  "native source helper parameters must be Long in this compiler slice") &&
               false;
    }
  }
  const XsSyntaxNode *block = xs_source_native_first_child_kind(function, XS_SYNTAX_STMT_BLOCK);
  if(block == nullptr || block->child_count == 0)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(function),
                              "native source main body must end with one return statement") &&
           false;
  for(size_t index = 0; index + 1 < block->child_count; ++index)
  {
    XsSyntaxKind kind = block->children[index]->kind;
    if(kind != XS_SYNTAX_STMT_VARIABLE && kind != XS_SYNTAX_STMT_EXPRESSION && kind != XS_SYNTAX_STMT_IF &&
       kind != XS_SYNTAX_STMT_WHILE && kind != XS_SYNTAX_STMT_FOR && kind != XS_SYNTAX_STMT_RETURN)
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(block->children[index]),
                                "native source main body supports only local declarations, assignments, if statements, "
                                "while statements, for statements, "
                                "and early return statements "
                                "before return for now") &&
             false;
  }
  const XsSyntaxNode *statement = block->children[block->child_count - 1];
  if(statement->kind != XS_SYNTAX_STMT_RETURN || statement->child_count != 1)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(statement),
                              "native source main body must end with one return expression") &&
           false;
  *body = block;
  return true;
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

static bool node_calls_function(const XsSyntaxNode *node, XsText name)
{
  if(node == nullptr)
    return false;
  const XsSyntaxNode *callee = simple_call_name(node);
  if(callee != nullptr && xs_source_native_text_equals(callee->text, name))
    return true;
  for(size_t index = 0; index < node->child_count; ++index)
  {
    if(node_calls_function(node->children[index], name))
      return true;
  }
  return false;
}

static bool reject_recursive_calls_from(const NativeProgram *program, size_t index, uint8_t *state,
                                        XsDiagnostics *diagnostics)
{
  state[index] = 1;
  const NativeFunction *function = &program->functions[index];
  for(size_t callee = 0; callee < program->count; ++callee)
  {
    if(!node_calls_function(function->body, program->functions[callee].name->text))
      continue;
    if(state[callee] == 1)
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(function->name),
                                "native source direct calls do not support recursion in this compiler slice") &&
             false;
    if(state[callee] == 0 && !reject_recursive_calls_from(program, callee, state, diagnostics))
      return false;
  }
  state[index] = 2;
  return true;
}

static bool reject_recursive_calls(const NativeProgram *program, XsDiagnostics *diagnostics)
{
  uint8_t state[32] = {0};
  for(size_t index = 0; index < program->count; ++index)
  {
    if(state[index] == 0 && !reject_recursive_calls_from(program, index, state, diagnostics))
      return false;
  }
  return true;
}

bool xs_source_native_collect_program(const XsSyntaxTree *tree, XsDiagnostics *diagnostics, NativeProgram *program)
{
  *program = (NativeProgram){0};
  size_t main_count = 0;
  for(size_t index = 0; index < tree->root->child_count; ++index)
  {
    const XsSyntaxNode *child = tree->root->children[index];
    if(child->kind != XS_SYNTAX_DECL_FUNCTION)
      continue;
    const XsSyntaxNode *name = xs_source_native_first_child_kind(child, XS_SYNTAX_IDENTIFIER);
    if(name == nullptr)
      continue;
    if(program->count == sizeof(program->functions) / sizeof(program->functions[0]))
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(child),
                                "native source build supports at most 32 functions for now") &&
             false;
    if(program_has_name(program, name->text))
      return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(name),
                                "native source build does not support overloaded or duplicate function names yet") &&
             false;
    bool is_main = xs_source_native_node_text_equals(name, "main");
    if(is_main)
      ++main_count;
    const XsSyntaxNode *body = nullptr;
    size_t parameter_count = 0;
    XsLilTypeKind return_kind = XS_LIL_TYPE_I32;
    if(!validate_shape(child, is_main, diagnostics, &body, &parameter_count, &return_kind))
      return false;
    program->functions[program->count++] = (NativeFunction){
        .function = child,
        .name = name,
        .body = body,
        .parameter_count = parameter_count,
        .return_kind = return_kind,
        .is_main = is_main,
    };
  }
  if(main_count == 1)
    return reject_recursive_calls(program, diagnostics);
  XsSpan span = tree->root == nullptr ? (XsSpan){0} : xs_source_native_node_span(tree->root);
  xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, span,
                     main_count == 0 ? "native source build requires one top-level 'main' function"
                                     : "native source build supports exactly one top-level 'main' function");
  return false;
}

static bool build_native_module(const char *input_path, const NativeProgram *program, XsDiagnostics *diagnostics)
{
  XsMirError mir_error = {0};
  XsMirModule *mir = nullptr;
  bool success = xs_mir_module_create("source", &mir, &mir_error) == XS_MIR_OK;
  for(size_t index = 0; success && index < program->count; ++index)
  {
    const NativeFunction *native = &program->functions[index];
    char *name = native->is_main ? xs_source_native_copy_cstr("main") : xs_source_native_copy_node_text(native->name);
    XsMirType parameters[16] = {0};
    if(name == nullptr)
    {
      success = false;
      xs_source_native_set_mir_error(&mir_error, XS_MIR_ALLOCATION_FAILED,
                                     "out of memory while naming native source function");
      break;
    }
    for(size_t parameter = 0; parameter < native->parameter_count; ++parameter)
      parameters[parameter] = (XsMirType){.kind = XS_LIL_TYPE_I32};
    success = xs_mir_module_add_function_definition(mir, name, (XsMirType){.kind = native->return_kind}, parameters,
                                                    native->parameter_count, nullptr, &mir_error) == XS_MIR_OK;
    free(name);
  }
  for(size_t index = 0; success && index < program->count; ++index)
  {
    XsMirFunction *function = (XsMirFunction *)(void *)xs_mir_module_function_at(mir, index);
    XsMirBlock *entry = nullptr;
    success = xs_mir_function_append_block(function, "entry", &entry, &mir_error) == XS_MIR_OK;
    if(success)
      success = xs_source_native_lower_function_body(function, entry, &program->functions[index], program, diagnostics,
                                                     &mir_error);
  }
  if(success)
    success = xs_mir_borrow_check_module(mir, &mir_error) == XS_MIR_OK;
  if(success)
    success = xs_mir_optimize_module_constants(mir, &mir_error) == XS_MIR_OK;
  if(success)
    success = xs_mir_optimize_module_cfg(mir, &mir_error) == XS_MIR_OK;
  if(success)
    success = xs_mir_borrow_check_module(mir, &mir_error) == XS_MIR_OK;
  if(!success)
  {
    fprintf(stderr, "xs: source native MIR build failed: %s\n", mir_error.message);
    xs_mir_module_destroy(mir);
    return false;
  }
  XsLilError lil_error = {0};
  XsLilModule *xlil = nullptr;
  success = xs_lil_module_create("source", &xlil, &lil_error) == XS_LIL_OK;
  if(success)
    success = xs_lil_module_add_mir_function_bodies(xlil, mir, &mir_error) == XS_MIR_OK;
  xs_mir_module_destroy(mir);
  if(!success)
  {
    fprintf(stderr, "xs: source native XLIL build failed: %s%s\n", lil_error.message, mir_error.message);
    xs_lil_module_destroy(xlil);
    return false;
  }
  success = xs_driver_build_lil_module_native(input_path, xlil);
  xs_lil_module_destroy(xlil);
  return success;
}

bool xs_driver_build_source_native(const char *input_path, const XsSyntaxTree *tree, XsDiagnostics *diagnostics)
{
  if(input_path == nullptr || tree == nullptr || tree->root == nullptr || diagnostics == nullptr)
  {
    fprintf(stderr, "xs: checked source tree is required for native source build\n");
    return false;
  }
  NativeProgram program = {0};
  if(!xs_source_native_collect_program(tree, diagnostics, &program))
    return false;
  return build_native_module(input_path, &program, diagnostics);
}
