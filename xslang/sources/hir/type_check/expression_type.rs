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
      Expression::Field { path } => Some(path.ty.clone()),
      Expression::Object { nominal_type, .. } => Some(Type::Named(nominal_type.clone())),
      Expression::Array { elements, .. } =>
      {
        let first = self.expression_type(elements.first()?)?;
        elements.iter()
                .skip(1)
                .all(|element| self.expression_type(element).as_ref() == Some(&first))
                .then(|| Type::Array { element: Box::new(first),
                                       length: u64::try_from(elements.len()).ok() })
      }
      Expression::Set { elements, .. } =>
      {
        let first = self.expression_type(elements.first()?)?;
        elements.iter()
                .skip(1)
                .all(|element| self.expression_type(element).as_ref() == Some(&first))
                .then(|| Type::Set { element: Box::new(first) })
      }
      Expression::Map { entries, .. } =>
      {
        let first = entries.first()?;
        let key = self.expression_type(&first.key)?;
        let value = self.expression_type(&first.value)?;
        entries.iter()
               .skip(1)
               .all(|entry| {
                 self.expression_type(&entry.key).as_ref() == Some(&key) &&
                 self.expression_type(&entry.value).as_ref() == Some(&value)
               })
               .then(|| Type::Map { key: Box::new(key),
                                    value: Box::new(value) })
      }
      Expression::Index { element_type, .. } => Some(element_type.as_ref().clone()),
      Expression::Assign { value, .. } => self.expression_type(value),
      Expression::AssignField { value, .. } => self.expression_type(value),
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
