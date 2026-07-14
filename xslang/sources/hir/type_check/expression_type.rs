/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl TypeChecker
{
  pub(super) fn expression_type(&self, expression: &Expression) -> Option<Type>
  {
    match expression
    {
      Expression::Literal { literal, .. } => literal_default_type(literal),
      Expression::Local { name, .. } => self.find_local(name).map(|local| local.ty.clone()),
      Expression::Assign { value, .. } => self.expression_type(value),
      Expression::Update { target, .. } => self.find_local(target).map(|local| local.ty.clone()),
      Expression::Binary { operator,
                           left,
                           right,
                           .. } => self.binary_expression_type(*operator, left, right),
      Expression::Unary { operator,
                          operand,
                          .. } => self.unary_expression_type(*operator, operand),
      Expression::ResultPropagation { value, .. } => self.result_success_type(value),
      Expression::Call { return_type, .. } |
      Expression::If { result_type: return_type,
                       .. } |
      Expression::Match { result_type: return_type,
                          .. } => Some(return_type.as_ref().clone()),
    }
  }
}
