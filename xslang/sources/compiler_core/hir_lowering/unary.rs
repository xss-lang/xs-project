/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn lower_unary_expression(tree: &SyntaxTree,
                                     value: &SyntaxNode,
                                     signatures: &HashMap<String, CallSignature>,
                                     locals: &HashMap<String, Type>,
                                     expected_type: Option<&Type>,
                                     source_span: Span)
                                     -> Option<Expression>
{
  let operator = match value.token_kind
  {
    TOKEN_BANG => UnaryOperator::LogicalNot,
    TOKEN_PLUS => UnaryOperator::Positive,
    TOKEN_MINUS => UnaryOperator::Negative,
    _ => return None,
  };
  let bool_type = Type::Primitive(PrimitiveType::Bool);
  let operand_type = if operator == UnaryOperator::LogicalNot
  {
    Some(&bool_type)
  }
  else
  {
    expected_type
  };
  let operand = lower_expression(tree,
                                 tree.nodes.get(value.children[0])?,
                                 signatures,
                                 locals,
                                 operand_type)?;
  Some(Expression::Unary { operator,
                           operand: Box::new(operand),
                           span: source_span })
}
