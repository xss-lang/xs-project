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
    let mut lowered = Function::definition(function.name.clone(), self.return_type(function, &local_types), vec![]);
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
      self.lower_terminator(block, xlil_block, &blocks, &values, &mut lowered);
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

  fn return_type(&mut self, function: &mir::Function, local_types: &HashMap<mir::LocalId, Option<Type>>) -> Type
  {
    for block in &function.blocks
    {
      let Some(mir::Terminator::Return(Some(local))) = block.terminator
      else
      {
        continue;
      };
      return match local_types.get(&local).copied().flatten()
      {
        Some(value_type) => value_type,
        None =>
        {
          self.report(DiagnosticCode::MissingLocalType,
                      "MIR return local has no XLIL value type",
                      block.span);
          Type::VOID
        }
      };
    }
    Type::VOID
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

  fn lower_terminator(&mut self,
                      block: &mir::BasicBlock,
                      xlil_block: BlockId,
                      blocks: &HashMap<mir::BlockId, BlockId>,
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
  use crate::mir::{BasicBlock, BlockId as MirBlockId, Function as MirFunction, Local, LocalId};
  use crate::xlil::{Instruction, Terminator};

  fn span(start: u32, end: u32) -> Span
  {
    Span::new(1, start, end)
  }

  #[test]
  fn lowers_void_return_function_skeleton()
  {
    let function = MirFunction { name: "Main".to_string(),
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
  fn lowers_goto_to_branch_terminator()
  {
    let function = MirFunction { name: "Branch".to_string(),
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
}
