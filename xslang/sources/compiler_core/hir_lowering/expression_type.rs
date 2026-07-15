/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn expression_type(tree: &SyntaxTree,
                              value: &SyntaxNode,
                              context: &LoweringContext,
                              locals: &HashMap<String, Type>)
                              -> Option<Type>
{
  match value.kind
  {
    kind if collection::is_expression(kind) => collection::expression_type(tree, value, context, locals),
    EXPR_LITERAL if value.text == "true" || value.text == "false" => Some(Type::Primitive(PrimitiveType::Bool)),
    EXPR_LITERAL if value.token_kind == TOKEN_INTEGER => Some(Type::Primitive(PrimitiveType::Int)),
    EXPR_LITERAL if value.token_kind == TOKEN_FLOAT => Some(Type::Primitive(PrimitiveType::Float)),
    EXPR_LITERAL if value.token_kind == TOKEN_CHARACTER => Some(Type::Primitive(PrimitiveType::Char)),
    EXPR_LITERAL if value.token_kind == TOKEN_STRING => Some(Type::Primitive(PrimitiveType::Str)),
    EXPR_IDENTIFIER => locals.get(&path_text(tree, value)).cloned(),
    EXPR_CALL =>
    {
      let callee = tree.nodes.get(*value.children.first()?)?;
      context.calls
             .get(&path_text(tree, callee))
             .map(|signature| signature.return_type.clone())
    }
    EXPR_BINARY
      if matches!(value.token_kind,
                  TOKEN_EQUAL |
                  TOKEN_NOT_EQUAL |
                  TOKEN_GREATER |
                  TOKEN_GREATER_EQUAL |
                  TOKEN_LESS |
                  TOKEN_LESS_EQUAL |
                  TOKEN_LOGICAL_AND |
                  TOKEN_LOGICAL_OR) =>
    {
      Some(Type::Primitive(PrimitiveType::Bool))
    }
    EXPR_BINARY => value.children
                        .first()
                        .and_then(|index| tree.nodes.get(*index))
                        .and_then(|operand| expression_type(tree, operand, context, locals))
                        .or_else(|| {
                          value.children
                               .last()
                               .and_then(|index| tree.nodes.get(*index))
                               .and_then(|operand| expression_type(tree, operand, context, locals))
                        })
                        .or(Some(Type::Primitive(PrimitiveType::Int))),
    EXPR_UNARY if value.token_kind == TOKEN_BANG => Some(Type::Primitive(PrimitiveType::Bool)),
    EXPR_UNARY => expression_type(tree, tree.nodes.get(*value.children.first()?)?, context, locals),
    _ => None,
  }
}
