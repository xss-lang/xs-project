/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::hir::type_check::TupleFieldValue;

impl Parser<'_>
{
  pub(super) fn tuple_expression(&mut self, type_text: &str) -> Option<Expression>
  {
    let tuple_type = self.parse_type(type_text)?;
    let Type::Tuple { fields: expected } = &tuple_type
    else
    {
      self.report("tuple expression requires a tuple type".to_string());
      return None;
    };
    self.index += 1;
    let mut fields = Vec::new();
    while let Some(line) = self.current()
    {
      if line == ".end"
      {
        self.index += 1;
        if fields.len() != expected.len()
        {
          self.report("tuple expression field count does not match its type".to_string());
          return None;
        }
        return Some(Expression::Tuple { fields,
                                        tuple_type: Box::new(tuple_type),
                                        span: span() });
      }
      let Some(name) = line.strip_prefix("field ")
      else
      {
        self.report(format!("expected tuple field, got '{line}'"));
        return None;
      };
      self.index += 1;
      fields.push(TupleFieldValue { name: (name != "positional").then(|| name.to_string()),
                                    value: self.expression()?,
                                    span: span() });
    }
    self.report("unterminated tuple expression".to_string());
    None
  }

  pub(super) fn tuple_element_expression(&mut self, record: &str) -> Option<Expression>
  {
    let (index, element_type) = record.split_once(' ')?;
    let index = index.parse().ok()?;
    let element_type = self.parse_type(element_type)?;
    self.index += 1;
    let tuple = self.expression()?;
    if self.current().as_deref() != Some(".end")
    {
      self.report("unterminated tuple element expression".to_string());
      return None;
    }
    self.index += 1;
    Some(Expression::TupleElement { tuple: Box::new(tuple),
                                    index,
                                    element_type: Box::new(element_type),
                                    span: span() })
  }
}
