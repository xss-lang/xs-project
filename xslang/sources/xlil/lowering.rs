/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use crate::hir::async_check::Span;
use crate::mir;
use crate::xlil::{BlockId, Function, Type, ValueId};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  UnsupportedReturnValue,
  MissingLocalType,
  UnsupportedLocalType,
  MissingLocalValue,
  MissingMirTerminator,
  MissingBranchTarget,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub message: String,
  pub span: Span,
}

#[derive(Default)]
pub struct MirToXlilLowerer
{
  diagnostics: Vec<Diagnostic>,
}

impl MirToXlilLowerer
{
  #[must_use]
  pub fn new() -> Self
  {
    Self::default()
  }

  pub fn lower_function(mut self, function: &mir::Function) -> Result<Function, Vec<Diagnostic>>
  {
    let mut local_types = local_types(function);
    let parameters = function.parameters
                             .iter()
                             .map(|parameter| parameter.value_type)
                             .collect();
    let mut lowered = Function::definition(function.name.clone(), function.return_type, parameters);
    let mut blocks = HashMap::new();
    let mut values = HashMap::new();
    for (index, parameter) in function.parameters.iter().enumerate()
    {
      local_types.insert(parameter.local, Some(parameter.value_type));
      if let Some(value) = lowered.parameter_value(index)
      {
        values.insert(parameter.local, value);
      }
    }
    for block in &function.blocks
    {
      let xlil_block = lowered.append_block(format!("bb{}", block.id.0));
      blocks.insert(block.id, xlil_block);
    }
    for block in &function.blocks
    {
      let xlil_block = blocks[&block.id];
      self.lower_statements(block, xlil_block, &local_types, &mut values, &mut lowered);
      self.lower_terminator(block, xlil_block, &blocks, &local_types, &values, &mut lowered);
    }
    if self.diagnostics.is_empty()
    {
      Ok(lowered)
    }
    else
    {
      Err(self.diagnostics)
    }
  }

  fn lower_statements(&mut self,
                      block: &mir::BasicBlock,
                      xlil_block: BlockId,
                      local_types: &HashMap<mir::LocalId, Option<Type>>,
                      values: &mut HashMap<mir::LocalId, ValueId>,
                      lowered: &mut Function)
  {
    for statement in &block.statements
    {
      if let mir::Statement::ConstI64 { local,
                                        value,
                                        span, } = *statement
      {
        self.lower_const_i64(local, value, span, xlil_block, local_types, values, lowered);
      }
      if let mir::Statement::ConstI32 { local,
                                        value,
                                        span, } = *statement
      {
        self.lower_const_i32(local, value, span, xlil_block, local_types, values, lowered);
      }
      if let mir::Statement::ConstBool { local,
                                         value,
                                         span, } = *statement
      {
        self.lower_const_bool(local, value, span, xlil_block, local_types, values, lowered);
      }
      if let mir::Statement::AddI64 { result,
                                      left,
                                      right,
                                      span, } = *statement
      {
        self.lower_binary_i64(result, left, right, span, "add.i64", xlil_block, values, lowered);
      }
      if let mir::Statement::SubI64 { result,
                                      left,
                                      right,
                                      span, } = *statement
      {
        self.lower_binary_i64(result, left, right, span, "sub.i64", xlil_block, values, lowered);
      }
      if let mir::Statement::MulI64 { result,
                                      left,
                                      right,
                                      span, } = *statement
      {
        self.lower_binary_i64(result, left, right, span, "mul.i64", xlil_block, values, lowered);
      }
      if let mir::Statement::EqI64 { result,
                                     left,
                                     right,
                                     span, } = *statement
      {
        self.lower_eq_i64(result, left, right, span, xlil_block, values, lowered);
      }
      if let mir::Statement::AddI32 { result,
                                      left,
                                      right,
                                      span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "add.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::SubI32 { result,
                                      left,
                                      right,
                                      span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "sub.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::MulI32 { result,
                                      left,
                                      right,
                                      span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "mul.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::EqI32 { result,
                                     left,
                                     right,
                                     span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "eq.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::LtI32 { result,
                                     left,
                                     right,
                                     span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "lt.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::LeI32 { result,
                                     left,
                                     right,
                                     span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "le.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::GtI32 { result,
                                     left,
                                     right,
                                     span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "gt.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::GeI32 { result,
                                     left,
                                     right,
                                     span, } = *statement
      {
        self.lower_binary_i32(result, left, right, span, "ge.i32", xlil_block, values, lowered);
      }
      if let mir::Statement::Call { result,
                                    ref function,
                                    ref arguments,
                                    return_type,
                                    span, } = *statement
      {
        self.lower_call(result,
                        function,
                        arguments,
                        return_type,
                        span,
                        xlil_block,
                        values,
                        lowered);
      }
    }
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_const_i64(&mut self,
                     local: mir::LocalId,
                     value: i64,
                     span: Span,
                     xlil_block: BlockId,
                     local_types: &HashMap<mir::LocalId, Option<Type>>,
                     values: &mut HashMap<mir::LocalId, ValueId>,
                     lowered: &mut Function)
  {
    match local_types.get(&local).copied().flatten()
    {
      Some(Type::I64) =>
      {
        if let Some(value_id) = lowered.add_const_i64(xlil_block, value)
        {
          values.insert(local, value_id);
        }
      }
      Some(_) => self.report(DiagnosticCode::UnsupportedLocalType,
                             "MIR const.i64 target local must have XLIL i64 type",
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "MIR const.i64 target local has no XLIL value type",
                          span),
    }
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_const_bool(&mut self,
                      local: mir::LocalId,
                      value: bool,
                      span: Span,
                      xlil_block: BlockId,
                      local_types: &HashMap<mir::LocalId, Option<Type>>,
                      values: &mut HashMap<mir::LocalId, ValueId>,
                      lowered: &mut Function)
  {
    match local_types.get(&local).copied().flatten()
    {
      Some(Type::BOOL) =>
      {
        if let Some(value_id) = lowered.add_const_bool(xlil_block, value)
        {
          values.insert(local, value_id);
        }
      }
      Some(_) => self.report(DiagnosticCode::UnsupportedLocalType,
                             "MIR const.bool target local must have XLIL bool type",
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "MIR const.bool target local has no XLIL value type",
                          span),
    }
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_const_i32(&mut self,
                     local: mir::LocalId,
                     value: i32,
                     span: Span,
                     xlil_block: BlockId,
                     local_types: &HashMap<mir::LocalId, Option<Type>>,
                     values: &mut HashMap<mir::LocalId, ValueId>,
                     lowered: &mut Function)
  {
    match local_types.get(&local).copied().flatten()
    {
      Some(Type::I32) =>
      {
        if let Some(value_id) = lowered.add_const_i32(xlil_block, value)
        {
          values.insert(local, value_id);
        }
      }
      Some(_) => self.report(DiagnosticCode::UnsupportedLocalType,
                             "MIR const.i32 target local must have XLIL i32 type",
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "MIR const.i32 target local has no XLIL value type",
                          span),
    }
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_call(&mut self,
                result: Option<mir::LocalId>,
                function: &str,
                arguments: &[mir::LocalId],
                return_type: Type,
                span: Span,
                xlil_block: BlockId,
                values: &mut HashMap<mir::LocalId, ValueId>,
                lowered: &mut Function)
  {
    let mut lowered_arguments = Vec::new();
    for argument in arguments
    {
      let Some(value) = values.get(argument).copied()
      else
      {
        self.report(DiagnosticCode::MissingLocalValue,
                    "MIR call argument does not have a lowered XLIL value",
                    span);
        return;
      };
      lowered_arguments.push(value);
    }
    let Some(call_result) = lowered.add_call(xlil_block, function, lowered_arguments, return_type)
    else
    {
      self.report(DiagnosticCode::UnsupportedReturnValue,
                  "MIR call could not be lowered to XLIL",
                  span);
      return;
    };
    if let Some(result) = result
    {
      let Some(call_result) = call_result
      else
      {
        self.report(DiagnosticCode::UnsupportedReturnValue,
                    "void MIR call cannot produce a result local",
                    span);
        return;
      };
      values.insert(result, call_result);
    }
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_binary_i64(&mut self,
                      result: mir::LocalId,
                      left: mir::LocalId,
                      right: mir::LocalId,
                      span: Span,
                      instruction: &str,
                      xlil_block: BlockId,
                      values: &mut HashMap<mir::LocalId, ValueId>,
                      lowered: &mut Function)
  {
    let Some(left) = values.get(&left).copied()
    else
    {
      self.report(DiagnosticCode::MissingLocalValue,
                  &format!("MIR {instruction} left operand does not have a lowered XLIL value"),
                  span);
      return;
    };
    let Some(right) = values.get(&right).copied()
    else
    {
      self.report(DiagnosticCode::MissingLocalValue,
                  &format!("MIR {instruction} right operand does not have a lowered XLIL value"),
                  span);
      return;
    };
    let value = match instruction
    {
      "add.i64" => lowered.add_i64(xlil_block, left, right),
      "sub.i64" => lowered.sub_i64(xlil_block, left, right),
      "mul.i64" => lowered.mul_i64(xlil_block, left, right),
      _ => None,
    };
    let Some(value) = value
    else
    {
      self.report(DiagnosticCode::UnsupportedLocalType,
                  &format!("MIR {instruction} operands must lower to XLIL i64 values"),
                  span);
      return;
    };
    values.insert(result, value);
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_eq_i64(&mut self,
                  result: mir::LocalId,
                  left: mir::LocalId,
                  right: mir::LocalId,
                  span: Span,
                  xlil_block: BlockId,
                  values: &mut HashMap<mir::LocalId, ValueId>,
                  lowered: &mut Function)
  {
    let Some(left) = values.get(&left).copied()
    else
    {
      self.report(DiagnosticCode::MissingLocalValue,
                  "MIR eq.i64 left operand does not have a lowered XLIL value",
                  span);
      return;
    };
    let Some(right) = values.get(&right).copied()
    else
    {
      self.report(DiagnosticCode::MissingLocalValue,
                  "MIR eq.i64 right operand does not have a lowered XLIL value",
                  span);
      return;
    };
    let Some(value) = lowered.eq_i64(xlil_block, left, right)
    else
    {
      self.report(DiagnosticCode::UnsupportedLocalType,
                  "MIR eq.i64 operands must lower to XLIL i64 values",
                  span);
      return;
    };
    values.insert(result, value);
  }

  #[allow(clippy::too_many_arguments)]
  fn lower_binary_i32(&mut self,
                      result: mir::LocalId,
                      left: mir::LocalId,
                      right: mir::LocalId,
                      span: Span,
                      instruction: &str,
                      xlil_block: BlockId,
                      values: &mut HashMap<mir::LocalId, ValueId>,
                      lowered: &mut Function)
  {
    let Some(left) = values.get(&left).copied()
    else
    {
      self.report(DiagnosticCode::MissingLocalValue,
                  &format!("MIR {instruction} left operand does not have a lowered XLIL value"),
                  span);
      return;
    };
    let Some(right) = values.get(&right).copied()
    else
    {
      self.report(DiagnosticCode::MissingLocalValue,
                  &format!("MIR {instruction} right operand does not have a lowered XLIL value"),
                  span);
      return;
    };
    let value = match instruction
    {
      "add.i32" => lowered.add_i32(xlil_block, left, right),
      "sub.i32" => lowered.sub_i32(xlil_block, left, right),
      "mul.i32" => lowered.mul_i32(xlil_block, left, right),
      "eq.i32" => lowered.eq_i32(xlil_block, left, right),
      "lt.i32" => lowered.lt_i32(xlil_block, left, right),
      "le.i32" => lowered.le_i32(xlil_block, left, right),
      "gt.i32" => lowered.gt_i32(xlil_block, left, right),
      "ge.i32" => lowered.ge_i32(xlil_block, left, right),
      _ => None,
    };
    let Some(value) = value
    else
    {
      self.report(DiagnosticCode::UnsupportedLocalType,
                  &format!("MIR {instruction} operands must lower to XLIL i32 values"),
                  span);
      return;
    };
    values.insert(result, value);
  }

  fn lower_terminator(&mut self,
                      block: &mir::BasicBlock,
                      xlil_block: BlockId,
                      blocks: &HashMap<mir::BlockId, BlockId>,
                      local_types: &HashMap<mir::LocalId, Option<Type>>,
                      values: &HashMap<mir::LocalId, ValueId>,
                      lowered: &mut Function)
  {
    match block.terminator
    {
      Some(mir::Terminator::Return(None)) =>
      {
        if !lowered.set_return(xlil_block, None)
        {
          self.report(DiagnosticCode::MissingMirTerminator,
                      "XLIL block could not receive a return terminator",
                      block.span);
        }
      }
      Some(mir::Terminator::Return(Some(local))) =>
      {
        if local_types.get(&local).copied().flatten().is_none()
        {
          self.report(DiagnosticCode::MissingLocalType,
                      "MIR return local has no XLIL value type",
                      block.span);
        }
        let Some(value) = values.get(&local).copied()
        else
        {
          self.report(DiagnosticCode::MissingLocalValue,
                      "MIR return local does not have a lowered XLIL value",
                      block.span);
          return;
        };
        if !lowered.set_return(xlil_block, Some(value))
        {
          self.report(DiagnosticCode::UnsupportedReturnValue,
                      "MIR return value does not match the lowered XLIL function signature",
                      block.span);
        }
      }
      Some(mir::Terminator::Goto(target)) =>
      {
        let Some(target) = blocks.get(&target).copied()
        else
        {
          self.report(DiagnosticCode::MissingBranchTarget,
                      "MIR goto target block is missing",
                      block.span);
          return;
        };
        if !lowered.set_branch(xlil_block, target)
        {
          self.report(DiagnosticCode::MissingBranchTarget,
                      "XLIL branch target block is missing",
                      block.span);
        }
      }
      Some(mir::Terminator::BranchIf { condition,
                                       then_block,
                                       else_block, }) =>
      {
        match local_types.get(&condition).copied().flatten()
        {
          Some(Type::BOOL) =>
          {}
          Some(_) =>
          {
            self.report(DiagnosticCode::UnsupportedLocalType,
                        "MIR branch_if condition local must have XLIL bool type",
                        block.span);
            return;
          }
          None => self.report(DiagnosticCode::MissingLocalType,
                              "MIR branch_if condition local has no XLIL value type",
                              block.span),
        }
        let Some(condition) = values.get(&condition).copied()
        else
        {
          self.report(DiagnosticCode::MissingLocalValue,
                      "MIR branch_if condition does not have a lowered XLIL value",
                      block.span);
          return;
        };
        let Some(then_block) = blocks.get(&then_block).copied()
        else
        {
          self.report(DiagnosticCode::MissingBranchTarget,
                      "MIR branch_if then target block is missing",
                      block.span);
          return;
        };
        let Some(else_block) = blocks.get(&else_block).copied()
        else
        {
          self.report(DiagnosticCode::MissingBranchTarget,
                      "MIR branch_if else target block is missing",
                      block.span);
          return;
        };
        if !lowered.set_branch_if(xlil_block, condition, then_block, else_block)
        {
          self.report(DiagnosticCode::MissingBranchTarget,
                      "XLIL conditional branch could not be created",
                      block.span);
        }
      }
      Some(mir::Terminator::Unreachable) | None => self.report(DiagnosticCode::MissingMirTerminator,
                                                               "MIR terminator cannot yet be lowered to XLIL",
                                                               block.span),
    }
  }

  fn report(&mut self, code: DiagnosticCode, message: &str, span: Span)
  {
    self.diagnostics.push(Diagnostic { code,
                                       message: message.to_string(),
                                       span });
  }
}

fn local_types(function: &mir::Function) -> HashMap<mir::LocalId, Option<Type>>
{
  function.locals
          .iter()
          .map(|local| (local.id, local.value_type))
          .collect()
}

#[cfg(test)]
mod tests;
