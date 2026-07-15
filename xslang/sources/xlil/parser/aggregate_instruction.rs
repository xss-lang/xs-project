/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{DiagnosticCode, Parser};
use crate::xlil::{Function, Instruction, TypeKind, Value};

impl Parser<'_>
{
  pub(super) fn aggregate_instruction(&mut self,
                                      function: &mut Function,
                                      result: &str,
                                      fields: &str,
                                      line: usize)
                                      -> Option<Instruction>
  {
    self.composite_instruction(function, result, fields, line, TypeKind::Aggregate)
  }

  pub(super) fn array_instruction(&mut self,
                                  function: &mut Function,
                                  result: &str,
                                  elements: &str,
                                  line: usize)
                                  -> Option<Instruction>
  {
    self.composite_instruction(function, result, elements, line, TypeKind::Array)
  }

  fn composite_instruction(&mut self,
                           function: &mut Function,
                           result: &str,
                           fields: &str,
                           line: usize,
                           expected_kind: TypeKind)
                           -> Option<Instruction>
  {
    let (result, result_type) = result.split_once(':')?;
    let result = self.value_id(result, line)?;
    let result_type = self.type_name(result_type, line)?;
    if result_type.kind != expected_kind
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL composite result must match its registry type");
      return None;
    }
    let mut parsed_fields = vec![];
    if !fields.is_empty()
    {
      for field in fields.split(", ")
      {
        parsed_fields.push(self.value_operand(field, line)?);
      }
    }
    function.values.push(Value { id: result,
                                 value_type: result_type });
    Some(Instruction::Aggregate { result,
                                  value_type: result_type,
                                  fields: parsed_fields })
  }

  pub(super) fn extract_instruction(&mut self,
                                    function: &mut Function,
                                    result: &str,
                                    source: &str,
                                    line: usize)
                                    -> Option<Instruction>
  {
    let (result, result_type) = result.split_once(':')?;
    let result = self.value_id(result, line)?;
    let result_type = self.type_name(result_type, line)?;
    let (aggregate, field) = source.split_once(", ")?;
    let aggregate = self.value_operand(aggregate, line)?;
    let field = field.parse::<u32>().ok()?;
    function.values.push(Value { id: result,
                                 value_type: result_type });
    Some(Instruction::Extract { result,
                                aggregate,
                                field })
  }
}
