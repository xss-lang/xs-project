/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::hir::type_check::MapEntry;

impl Parser<'_>
{
  pub(super) fn array_expression(&mut self) -> Option<Expression>
  {
    self.index += 1;
    let mut elements = Vec::new();
    while let Some(line) = self.current()
    {
      if line == ".end"
      {
        self.index += 1;
        return Some(Expression::Array { elements,
                                        span: span() });
      }
      if line != "element"
      {
        self.report(format!("expected array element, got '{line}'"));
        return None;
      }
      self.index += 1;
      elements.push(self.expression()?);
    }
    self.report("unterminated array expression".to_string());
    None
  }

  pub(super) fn map_expression(&mut self) -> Option<Expression>
  {
    self.index += 1;
    let mut entries = Vec::new();
    while let Some(line) = self.current()
    {
      if line == ".end"
      {
        self.index += 1;
        return Some(Expression::Map { entries,
                                      span: span() });
      }
      if line != "entry"
      {
        self.report(format!("expected map entry, got '{line}'"));
        return None;
      }
      self.index += 1;
      self.consume_expression_field("key");
      let key = self.expression()?;
      self.consume_expression_field("value");
      let value = self.expression()?;
      entries.push(MapEntry { key,
                              value,
                              span: span() });
    }
    self.report("unterminated map expression".to_string());
    None
  }

  pub(super) fn index_expression(&mut self, element_type: &str) -> Option<Expression>
  {
    let element_type = self.parse_type(element_type)?;
    self.index += 1;
    self.consume_expression_field("collection");
    let collection = self.expression()?;
    self.consume_expression_field("offset");
    let index = self.expression()?;
    if self.current().as_deref() != Some(".end")
    {
      self.report("unterminated index expression".to_string());
      return None;
    }
    self.index += 1;
    Some(Expression::Index { collection: Box::new(collection),
                             index: Box::new(index),
                             element_type: Box::new(element_type),
                             span: span() })
  }
}
