/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn lower_array_expression(&mut self,
                                       expression: &Expression,
                                       expected_type: XlilType,
                                       lowered: &mut mir::Function)
                                       -> Option<mir::LocalId>
  {
    let Expression::Array { elements,
                            span, } = expression
    else
    {
      return None;
    };
    let Some((_, element_type, length)) = self.array_layouts
                                              .iter()
                                              .find(|(value_type, _, _)| *value_type == expected_type)
                                              .copied()
    else
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "array literal target has no fixed-array MIR layout",
                  *span);
      return None;
    };
    if u64::try_from(elements.len()).ok() != Some(length)
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "array literal length does not match its fixed-array type",
                  *span);
      return None;
    }
    let fields = elements.iter()
                         .map(|element| self.lower_expression_to_local(element, element_type, lowered))
                         .collect::<Option<Vec<_>>>()?;
    let result = self.declare_temp(expected_type, *span, lowered)?;
    self.current_block_mut(lowered)
        .statements
        .push(mir::Statement::Aggregate { result,
                                          value_type: expected_type,
                                          field_types: vec![element_type; fields.len()],
                                          fields,
                                          span: *span });
    Some(result)
  }

  pub(super) fn lower_index_expression(&mut self,
                                       expression: &Expression,
                                       expected_type: XlilType,
                                       lowered: &mut mir::Function)
                                       -> Option<mir::LocalId>
  {
    let Expression::Index { collection,
                            index,
                            span,
                            .. } = expression
    else
    {
      return None;
    };
    let collection_type = self.expression_value_type(collection, lowered)?;
    let Some((_, element_type, length)) = self.array_layouts
                                              .iter()
                                              .find(|(value_type, _, _)| *value_type == collection_type)
                                              .copied()
    else
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "index source has no fixed-array MIR layout",
                  *span);
      return None;
    };
    if element_type != expected_type
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "index result type does not match its fixed-array element type",
                  *span);
      return None;
    }
    let Expression::Literal { literal: Literal::Integer(index),
                              .. } = index.as_ref()
    else
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "fixed-array MIR indexing currently requires an integer literal",
                  *span);
      return None;
    };
    let Ok(index) = index.replace('\'', "").parse::<u32>()
    else
    {
      self.report(DiagnosticCode::InvalidIntegerLiteral,
                  "array index is not a valid u32 literal",
                  *span);
      return None;
    };
    if u64::from(index) >= length
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "fixed-array index is out of bounds",
                  *span);
      return None;
    }
    let array = self.lower_expression_to_local(collection, collection_type, lowered)?;
    let result = self.declare_temp(element_type, *span, lowered)?;
    self.current_block_mut(lowered)
        .statements
        .push(mir::Statement::Extract { result,
                                        aggregate: array,
                                        field: index,
                                        field_type: element_type,
                                        span: *span });
    Some(result)
  }
}
