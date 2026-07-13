/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl TypeChecker
{
  pub(super) fn unary_expression_type(&self, operator: UnaryOperator, operand: &Expression) -> Option<Type>
  {
    let Type::Primitive(primitive) = self.expression_type(operand)?
    else
    {
      return None;
    };
    match operator
    {
      UnaryOperator::Positive | UnaryOperator::Negative
        if matches!(primitive, PrimitiveType::Long | PrimitiveType::Int) =>
      {
        Some(Type::Primitive(primitive))
      }
      UnaryOperator::LogicalNot if primitive == PrimitiveType::Bool => Some(Type::Primitive(PrimitiveType::Bool)),
      _ => None,
    }
  }

  pub(super) fn unary_expression_matches_type(&self,
                                              operator: UnaryOperator,
                                              operand: &Expression,
                                              expected: &Type)
                                              -> bool
  {
    match (operator, expected)
    {
      (UnaryOperator::Positive | UnaryOperator::Negative,
       Type::Primitive(PrimitiveType::Long | PrimitiveType::Int)) => match operand
      {
        Expression::Literal { literal, .. } => literal_matches_type(literal, expected),
        Expression::Unary { operator,
                            operand,
                            .. } => self.unary_expression_matches_type(*operator, operand, expected),
        _ => self.expression_type(operand).as_ref() == Some(expected),
      },
      (UnaryOperator::LogicalNot, Type::Primitive(PrimitiveType::Bool)) =>
      {
        self.expression_type(operand).as_ref() == Some(expected)
      }
      _ => false,
    }
  }
}
