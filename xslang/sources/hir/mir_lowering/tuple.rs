/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn lower_tuple_expression(&mut self,
                                       expression: &Expression,
                                       expected_type: XlilType,
                                       lowered: &mut mir::Function)
                                       -> Option<mir::LocalId>
  {
    let Expression::Tuple { fields,
                            tuple_type,
                            span, } = expression
    else
    {
      return None;
    };
    if self.lower_value_type(tuple_type, *span)? != expected_type
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "tuple literal type does not match its MIR target type",
                  *span);
      return None;
    }
    let Type::Tuple { fields: field_types } = tuple_type.as_ref()
    else
    {
      return None;
    };
    if fields.len() != field_types.len()
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "tuple literal field count does not match its HIR type",
                  *span);
      return None;
    }
    let mut values = Vec::with_capacity(fields.len());
    let mut lowered_types = Vec::with_capacity(fields.len());
    for (field, field_type) in fields.iter().zip(field_types)
    {
      if field.name != field_type.name
      {
        self.report(DiagnosticCode::UnsupportedExpression,
                    "tuple literal field name does not match its HIR type",
                    field.span);
        return None;
      }
      let value_type = self.lower_value_type(&field_type.ty, field.span)?;
      values.push(self.lower_expression_to_local(&field.value, value_type, lowered)?);
      lowered_types.push(value_type);
    }
    let result = self.declare_temp(expected_type, *span, lowered)?;
    self.current_block_mut(lowered)
        .statements
        .push(mir::Statement::Aggregate { result,
                                          value_type: expected_type,
                                          fields: values,
                                          field_types: lowered_types,
                                          span: *span });
    Some(result)
  }

  pub(super) fn lower_tuple_element(&mut self,
                                    expression: &Expression,
                                    expected_type: XlilType,
                                    lowered: &mut mir::Function)
                                    -> Option<mir::LocalId>
  {
    let Expression::TupleElement { tuple,
                                   index,
                                   element_type,
                                   span, } = expression
    else
    {
      return None;
    };
    if self.lower_value_type(element_type, *span)? != expected_type
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "tuple element type does not match its MIR target type",
                  *span);
      return None;
    }
    let tuple_type = self.expression_value_type(tuple, lowered)?;
    let tuple_value = self.lower_expression_to_local(tuple, tuple_type, lowered)?;
    let result = self.declare_temp(expected_type, *span, lowered)?;
    self.current_block_mut(lowered)
        .statements
        .push(mir::Statement::Extract { result,
                                        aggregate: tuple_value,
                                        field: *index,
                                        field_type: expected_type,
                                        span: *span });
    Some(result)
  }
}
