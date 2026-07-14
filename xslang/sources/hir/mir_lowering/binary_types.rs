/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn binary_operand_type(&self,
                                    operator: BinaryOperator,
                                    target_type: XlilType,
                                    left: &Expression,
                                    right: &Expression,
                                    lowered: &mir::Function)
                                    -> Option<XlilType>
  {
    match (operator, target_type)
    {
      (operator, value_type) if value_type.is_integer() && integer_operation(operator).is_some() => Some(value_type),
      (BinaryOperator::Add | BinaryOperator::Sub | BinaryOperator::Mul, XlilType::I32) => Some(XlilType::I32),
      (operator, XlilType::I32) if binary_i32_operation(operator).is_some() => Some(XlilType::I32),
      (BinaryOperator::Add | BinaryOperator::Sub | BinaryOperator::Mul, XlilType::I64) => Some(XlilType::I64),
      (operator, XlilType::I64) if binary_i64_operation(operator).is_some() => Some(XlilType::I64),
      (operator, XlilType::F32 | XlilType::F64) if binary_float_operation(operator).is_some() => Some(target_type),
      (BinaryOperator::Equal |
       BinaryOperator::NotEqual |
       BinaryOperator::Less |
       BinaryOperator::LessEqual |
       BinaryOperator::Greater |
       BinaryOperator::GreaterEqual,
       XlilType::BOOL) => self.common_operand_type(left, right, lowered).or(Some(XlilType::I64)),
      _ => None,
    }
  }

  fn common_operand_type(&self, left: &Expression, right: &Expression, lowered: &mir::Function) -> Option<XlilType>
  {
    let left_type = self.expression_value_type(left, lowered);
    let right_type = self.expression_value_type(right, lowered);
    match (left_type, right_type)
    {
      (Some(left_type), Some(right_type)) if left_type == right_type => Some(left_type),
      (Some(value_type), None) | (None, Some(value_type)) => Some(value_type),
      _ => None,
    }
  }

  pub(super) fn expression_value_type(&self, expression: &Expression, lowered: &mir::Function) -> Option<XlilType>
  {
    match expression
    {
      Expression::Local { name, .. } => self.local_value_type(*self.locals.get(name)?, lowered),
      Expression::Update { target, .. } => self.local_value_type(*self.locals.get(target)?, lowered),
      Expression::Binary { operator,
                           left,
                           right,
                           .. } => match operator
      {
        BinaryOperator::Add |
        BinaryOperator::Sub |
        BinaryOperator::Mul |
        BinaryOperator::Div |
        BinaryOperator::Rem |
        BinaryOperator::BitAnd |
        BinaryOperator::BitOr |
        BinaryOperator::BitXor |
        BinaryOperator::ShiftLeft |
        BinaryOperator::ShiftRight => self.common_operand_type(left, right, lowered),
        BinaryOperator::LogicalAnd | BinaryOperator::LogicalOr => Some(XlilType::BOOL),
        BinaryOperator::Equal |
        BinaryOperator::NotEqual |
        BinaryOperator::Less |
        BinaryOperator::LessEqual |
        BinaryOperator::Greater |
        BinaryOperator::GreaterEqual => Some(XlilType::BOOL),
      },
      Expression::Unary { operator,
                          operand,
                          .. } => match operator
      {
        UnaryOperator::LogicalNot => Some(XlilType::BOOL),
        UnaryOperator::Positive | UnaryOperator::Negative => self.expression_value_type(operand, lowered),
      },
      Expression::Call { return_type, .. } => type_to_xlil(return_type),
      Expression::If { result_type, .. } | Expression::Match { result_type, .. } => type_to_xlil(result_type),
      Expression::Literal { .. } | Expression::Assign { .. } | Expression::ResultPropagation { .. } => None,
    }
  }
}

fn type_to_xlil(value: &Type) -> Option<XlilType>
{
  match value
  {
    Type::Primitive(value) => primitive_to_xlil(*value),
    Type::Unit | Type::Named(_) => None,
  }
}
