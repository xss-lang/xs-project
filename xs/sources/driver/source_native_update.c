/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "source_native_internal.h"

bool xs_source_native_lower_update_value(XsMirBlock *block, const NativeContext *context,
                                         const XsSyntaxNode *expression, XsDiagnostics *diagnostics,
                                         XsMirValueId *result, XsMirError *error)
{
  if(expression == nullptr || expression->kind != XS_SYNTAX_EXPR_UNARY || expression->child_count != 1 ||
     (expression->token_kind != XS_TOKEN_PLUS_PLUS && expression->token_kind != XS_TOKEN_MINUS_MINUS) ||
     expression->children[0]->kind != XS_SYNTAX_EXPR_IDENTIFIER)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(expression),
                              "native source increment/decrement requires a Long local") &&
           false;

  const XsSyntaxNode *target = expression->children[0];
  XsMirValueId current = 0;
  XsMirValueId one = 0;
  XsMirValueId updated = 0;
  if(!xs_source_native_context_read(context, block, target->text, XS_LIL_TYPE_I32, &current, error) ||
     xs_mir_block_add_const_i32(block, 1, &one, error) != XS_MIR_OK)
    return xs_diagnostics_add(diagnostics, XS_DIAGNOSTIC_ERROR, xs_source_native_node_span(target),
                              "native source increment/decrement requires a Long local") &&
           false;

  XsMirStatus status = expression->token_kind == XS_TOKEN_PLUS_PLUS
                           ? xs_mir_block_add_i32(block, current, one, &updated, error)
                           : xs_mir_block_sub_i32(block, current, one, &updated, error);
  if(status != XS_MIR_OK || !xs_source_native_context_assign(context, block, target->text, XS_LIL_TYPE_I32, updated,
                                                             diagnostics, xs_source_native_node_span(target), error))
    return false;

  *result = (expression->flags & XS_SYNTAX_FLAG_PREFIX_UPDATE) != 0 ? updated : current;
  return true;
}
