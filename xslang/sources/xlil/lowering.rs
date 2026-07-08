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
    let local_types = local_types(function);
    let parameters = function.parameters
                             .iter()
                             .map(|parameter| parameter.value_type)
                             .collect();
    let mut lowered = Function::definition(function.name.clone(), function.return_type, parameters);
    let mut blocks = HashMap::new();
    let mut values = HashMap::new();
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
          Some(_) => self.report(DiagnosticCode::UnsupportedLocalType,
                                 "MIR branch_if condition local must have XLIL bool type",
                                 block.span),
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
mod tests
{
  use super::*;
  use crate::mir::{BasicBlock, BlockId as MirBlockId, Function as MirFunction, Local, LocalId, Parameter};
  use crate::xlil::{Instruction, Terminator};

  fn span(start: u32, end: u32) -> Span
  {
    Span::new(1, start, end)
  }

  #[test]
  fn lowers_void_return_function_skeleton()
  {
    let function = MirFunction { name: "Main".to_string(),
                                 parameters: vec![],
                                 return_type: Type::VOID,
                                 locals: vec![],
                                 blocks: vec![BasicBlock { id: MirBlockId(0),
                                                           statements: vec![],
                                                           terminator: Some(mir::Terminator::Return(None)),
                                                           span: span(0, 1) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.name, "Main");
    assert_eq!(lowered.blocks.len(), 1);
    assert_eq!(lowered.blocks[0].terminator, Some(Terminator::Return(None)));
  }

  #[test]
  fn rejects_return_value_without_type_and_value_mapping()
  {
    let function = MirFunction { name: "Value".to_string(),
                                 parameters: vec![],
                                 return_type: Type::I64,
                                 locals: vec![],
                                 blocks: vec![BasicBlock { id: MirBlockId(0),
                                                           statements: vec![],
                                                           terminator:
                                                             Some(mir::Terminator::Return(Some(LocalId(0)))),
                                                           span: span(0, 1) }] };

    let diagnostics = MirToXlilLowerer::new().lower_function(&function)
                                             .expect_err("lowering must diagnose");

    assert!(diagnostics.iter()
                       .any(|diagnostic| diagnostic.code == DiagnosticCode::MissingLocalType));
    assert!(diagnostics.iter()
                       .any(|diagnostic| diagnostic.code == DiagnosticCode::MissingLocalValue));
  }

  #[test]
  fn lowers_i64_const_and_return_value()
  {
    let function =
      MirFunction { name: "Value".to_string(),
                    parameters: vec![],
                    return_type: Type::I64,
                    locals: vec![Local { id: LocalId(0),
                                         name: "answer".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) }],
                    blocks: vec![BasicBlock { id: MirBlockId(0),
                                              statements: vec![mir::Statement::ConstI64 { local: LocalId(0),
                                                                                          value: 42,
                                                                                          span: span(1, 2) }],
                                              terminator: Some(mir::Terminator::Return(Some(LocalId(0)))),
                                              span: span(0, 3) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.return_type, Type::I64);
    assert_eq!(lowered.blocks[0].instructions,
               vec![Instruction::ConstI64 { result: crate::xlil::ValueId(0),
                                            value: 42 }]);
    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Return(Some(crate::xlil::ValueId(0)))));
  }

  #[test]
  fn lowers_call_statement()
  {
    let function = MirFunction { name: "CallValue".to_string(),
                                 parameters: vec![],
                                 return_type: Type::I64,
                                 locals: vec![Local { id: LocalId(0),
                                                      name: "argument".to_string(),
                                                      value_type: Some(Type::I64),
                                                      mutable: false,
                                                      span: span(0, 1) },
                                              Local { id: LocalId(1),
                                                      name: "result".to_string(),
                                                      value_type: Some(Type::I64),
                                                      mutable: false,
                                                      span: span(0, 1) }],
                                 blocks: vec![BasicBlock { id: MirBlockId(0),
                                                           statements:
                                                             vec![mir::Statement::ConstI64 { local: LocalId(0),
                                                                                             value: 7,
                                                                                             span: span(1, 2) },
                                                                  mir::Statement::Call { result: Some(LocalId(1)),
                                                                                         function:
                                                                                           "xs$App$Callee".to_string(),
                                                                                         arguments: vec![LocalId(0)],
                                                                                         return_type: Type::I64,
                                                                                         span: span(2, 3) }],
                                                           terminator:
                                                             Some(mir::Terminator::Return(Some(LocalId(1)))),
                                                           span: span(0, 4) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].instructions.len(), 2);
    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Return(Some(crate::xlil::ValueId(1)))));
  }

  #[test]
  fn lowers_add_i64_statement()
  {
    let function =
      MirFunction { name: "Add".to_string(),
                    parameters: vec![],
                    return_type: Type::I64,
                    locals: vec![Local { id: LocalId(0),
                                         name: "left".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(1),
                                         name: "right".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(2),
                                         name: "sum".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) }],
                    blocks: vec![BasicBlock { id: MirBlockId(0),
                                              statements: vec![mir::Statement::ConstI64 { local: LocalId(0),
                                                                                          value: 2,
                                                                                          span: span(1, 2) },
                                                               mir::Statement::ConstI64 { local: LocalId(1),
                                                                                          value: 3,
                                                                                          span: span(2, 3) },
                                                               mir::Statement::AddI64 { result: LocalId(2),
                                                                                        left: LocalId(0),
                                                                                        right: LocalId(1),
                                                                                        span: span(3, 4) }],
                                              terminator: Some(mir::Terminator::Return(Some(LocalId(2)))),
                                              span: span(0, 5) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].instructions[2], Instruction::AddI64 { result:
                                                                          crate::xlil::ValueId(2),
                                                                        left:
                                                                          crate::xlil::ValueId(0),
                                                                        right:
                                                                          crate::xlil::ValueId(1) });
    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Return(Some(crate::xlil::ValueId(2)))));
  }

  #[test]
  fn lowers_sub_i64_statement()
  {
    let function =
      MirFunction { name: "Sub".to_string(),
                    parameters: vec![],
                    return_type: Type::I64,
                    locals: vec![Local { id: LocalId(0),
                                         name: "left".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(1),
                                         name: "right".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(2),
                                         name: "difference".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) }],
                    blocks: vec![BasicBlock { id: MirBlockId(0),
                                              statements: vec![mir::Statement::ConstI64 { local: LocalId(0),
                                                                                          value: 8,
                                                                                          span: span(1, 2) },
                                                               mir::Statement::ConstI64 { local: LocalId(1),
                                                                                          value: 3,
                                                                                          span: span(2, 3) },
                                                               mir::Statement::SubI64 { result: LocalId(2),
                                                                                        left: LocalId(0),
                                                                                        right: LocalId(1),
                                                                                        span: span(3, 4) }],
                                              terminator: Some(mir::Terminator::Return(Some(LocalId(2)))),
                                              span: span(0, 5) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].instructions[2], Instruction::SubI64 { result:
                                                                          crate::xlil::ValueId(2),
                                                                        left:
                                                                          crate::xlil::ValueId(0),
                                                                        right:
                                                                          crate::xlil::ValueId(1) });
    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Return(Some(crate::xlil::ValueId(2)))));
  }

  #[test]
  fn lowers_mul_i64_statement()
  {
    let function =
      MirFunction { name: "Mul".to_string(),
                    parameters: vec![],
                    return_type: Type::I64,
                    locals: vec![Local { id: LocalId(0),
                                         name: "left".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(1),
                                         name: "right".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(2),
                                         name: "product".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) }],
                    blocks: vec![BasicBlock { id: MirBlockId(0),
                                              statements: vec![mir::Statement::ConstI64 { local: LocalId(0),
                                                                                          value: 6,
                                                                                          span: span(1, 2) },
                                                               mir::Statement::ConstI64 { local: LocalId(1),
                                                                                          value: 7,
                                                                                          span: span(2, 3) },
                                                               mir::Statement::MulI64 { result: LocalId(2),
                                                                                        left: LocalId(0),
                                                                                        right: LocalId(1),
                                                                                        span: span(3, 4) }],
                                              terminator: Some(mir::Terminator::Return(Some(LocalId(2)))),
                                              span: span(0, 5) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].instructions[2], Instruction::MulI64 { result:
                                                                          crate::xlil::ValueId(2),
                                                                        left:
                                                                          crate::xlil::ValueId(0),
                                                                        right:
                                                                          crate::xlil::ValueId(1) });
    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Return(Some(crate::xlil::ValueId(2)))));
  }

  #[test]
  fn lowers_const_bool_and_eq_i64_statement()
  {
    let function =
      MirFunction { name: "Eq".to_string(),
                    parameters: vec![],
                    return_type: Type::BOOL,
                    locals: vec![Local { id: LocalId(0),
                                         name: "left".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(1),
                                         name: "right".to_string(),
                                         value_type: Some(Type::I64),
                                         mutable: false,
                                         span: span(0, 1) },
                                 Local { id: LocalId(2),
                                         name: "same".to_string(),
                                         value_type: Some(Type::BOOL),
                                         mutable: false,
                                         span: span(0, 1) }],
                    blocks: vec![BasicBlock { id: MirBlockId(0),
                                              statements: vec![mir::Statement::ConstI64 { local: LocalId(0),
                                                                                          value: 7,
                                                                                          span: span(1, 2) },
                                                               mir::Statement::ConstI64 { local: LocalId(1),
                                                                                          value: 7,
                                                                                          span: span(2, 3) },
                                                               mir::Statement::EqI64 { result: LocalId(2),
                                                                                       left: LocalId(0),
                                                                                       right: LocalId(1),
                                                                                       span: span(3, 4) }],
                                              terminator: Some(mir::Terminator::Return(Some(LocalId(2)))),
                                              span: span(0, 5) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].instructions[2], Instruction::EqI64 { result:
                                                                         crate::xlil::ValueId(2),
                                                                       left:
                                                                         crate::xlil::ValueId(0),
                                                                       right:
                                                                         crate::xlil::ValueId(1) });
    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Return(Some(crate::xlil::ValueId(2)))));
  }

  #[test]
  fn lowers_signature_parameters()
  {
    let function = MirFunction { name: "WithParameter".to_string(),
                                 parameters: vec![Parameter { name: "value".to_string(),
                                                              value_type: Type::I64,
                                                              span: span(0, 1) }],
                                 return_type: Type::VOID,
                                 locals: vec![],
                                 blocks: vec![BasicBlock { id: MirBlockId(0),
                                                           statements: vec![],
                                                           terminator: Some(mir::Terminator::Return(None)),
                                                           span: span(0, 1) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.parameters, vec![Type::I64]);
  }

  #[test]
  fn lowers_goto_to_branch_terminator()
  {
    let function = MirFunction { name: "Branch".to_string(),
                                 parameters: vec![],
                                 return_type: Type::VOID,
                                 locals: vec![],
                                 blocks: vec![BasicBlock { id: MirBlockId(0),
                                                           statements: vec![],
                                                           terminator:
                                                             Some(mir::Terminator::Goto(MirBlockId(1))),
                                                           span: span(0, 1) },
                                              BasicBlock { id: MirBlockId(1),
                                                           statements: vec![],
                                                           terminator: Some(mir::Terminator::Return(None)),
                                                           span: span(1, 2) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::Branch(crate::xlil::BlockId(1))));
  }

  #[test]
  fn lowers_branch_if_terminator()
  {
    let function = MirFunction { name: "BranchIf".to_string(),
                                 parameters: vec![],
                                 return_type: Type::VOID,
                                 locals: vec![Local { id: LocalId(0),
                                                      name: "condition".to_string(),
                                                      value_type: Some(Type::BOOL),
                                                      mutable: false,
                                                      span: span(0, 1) }],
                                 blocks: vec![BasicBlock { id: MirBlockId(0),
                                                           statements:
                                                             vec![mir::Statement::ConstBool { local: LocalId(0),
                                                                                              value: true,
                                                                                              span: span(1, 2) }],
                                                           terminator:
                                                             Some(mir::Terminator::BranchIf { condition: LocalId(0),
                                                                                              then_block:
                                                                                                MirBlockId(1),
                                                                                              else_block:
                                                                                                MirBlockId(2) }),
                                                           span: span(0, 3) },
                                              BasicBlock { id: MirBlockId(1),
                                                           statements: vec![],
                                                           terminator: Some(mir::Terminator::Return(None)),
                                                           span: span(3, 4) },
                                              BasicBlock { id: MirBlockId(2),
                                                           statements: vec![],
                                                           terminator: Some(mir::Terminator::Return(None)),
                                                           span: span(4, 5) }] };

    let lowered = MirToXlilLowerer::new().lower_function(&function)
                                         .expect("lowering should succeed");

    assert_eq!(lowered.blocks[0].terminator,
               Some(Terminator::BranchIf { condition: crate::xlil::ValueId(0),
                                           then_block: crate::xlil::BlockId(1),
                                           else_block: crate::xlil::BlockId(2) }));
  }
}
