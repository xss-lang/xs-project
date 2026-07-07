/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use crate::hir::async_check::Span;
use crate::mir;
use crate::xlil::{BlockId, Function, Type};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  UnsupportedReturnValue,
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
    let mut lowered = Function::definition(function.name.clone(), Type::VOID, vec![]);
    let mut blocks = HashMap::new();
    for block in &function.blocks
    {
      let xlil_block = lowered.append_block(format!("bb{}", block.id.0));
      blocks.insert(block.id, xlil_block);
    }
    for block in &function.blocks
    {
      let xlil_block = blocks[&block.id];
      self.lower_terminator(block, xlil_block, &blocks, &mut lowered);
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

  fn lower_terminator(&mut self,
                      block: &mir::BasicBlock,
                      xlil_block: BlockId,
                      blocks: &HashMap<mir::BlockId, BlockId>,
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
      Some(mir::Terminator::Return(Some(_))) => self.report(DiagnosticCode::UnsupportedReturnValue,
                                                            "MIR return values are lowered after value mapping exists",
                                                            block.span),
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

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::mir::{BasicBlock, BlockId as MirBlockId, Function as MirFunction, LocalId};
  use crate::xlil::Terminator;

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
  fn rejects_return_value_until_value_mapping_exists()
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

    assert_eq!(diagnostics.len(), 1);
    assert_eq!(diagnostics[0].code, DiagnosticCode::UnsupportedReturnValue);
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
