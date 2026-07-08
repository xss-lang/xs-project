/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use super::verify::{Diagnostic as VerifyDiagnostic, verify_function};
use super::{BasicBlock, Function, LocalId, Statement, reachable_blocks};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum OptimizationPass
{
  RemoveUnreachableBlocks,
  RemoveRedundantEndBorrow,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct OptimizationReport
{
  pub pass: OptimizationPass,
  pub removed_items: usize,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct OptimizedFunction
{
  pub function: Function,
  pub reports: Vec<OptimizationReport>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum OptimizationError
{
  InputInvalid(Vec<VerifyDiagnostic>),
  OutputInvalid(Vec<VerifyDiagnostic>),
}

#[must_use]
pub fn optimize_function(mut function: Function) -> OptimizedFunction
{
  let mut reports = Vec::new();
  let removed_blocks = remove_unreachable_blocks(&mut function);
  if removed_blocks != 0
  {
    reports.push(OptimizationReport { pass: OptimizationPass::RemoveUnreachableBlocks,
                                      removed_items: removed_blocks });
  }
  let removed_end_borrows = remove_redundant_end_borrows(&mut function);
  if removed_end_borrows != 0
  {
    reports.push(OptimizationReport { pass: OptimizationPass::RemoveRedundantEndBorrow,
                                      removed_items: removed_end_borrows });
  }
  OptimizedFunction { function,
                      reports }
}

pub fn optimize_verified_function(function: Function) -> Result<OptimizedFunction, OptimizationError>
{
  let input_diagnostics = verify_function(&function);
  if !input_diagnostics.is_empty()
  {
    return Err(OptimizationError::InputInvalid(input_diagnostics));
  }
  let optimized = optimize_function(function);
  let output_diagnostics = verify_function(&optimized.function);
  if !output_diagnostics.is_empty()
  {
    return Err(OptimizationError::OutputInvalid(output_diagnostics));
  }
  Ok(optimized)
}

fn remove_unreachable_blocks(function: &mut Function) -> usize
{
  let reachable = reachable_blocks(function);
  let before = function.blocks.len();
  function.blocks.retain(|block| reachable.contains(&block.id));
  before - function.blocks.len()
}

fn remove_redundant_end_borrows(function: &mut Function) -> usize
{
  let mut removed = 0;
  for block in &mut function.blocks
  {
    removed += remove_redundant_end_borrows_in_block(block);
  }
  removed
}

fn remove_redundant_end_borrows_in_block(block: &mut BasicBlock) -> usize
{
  let mut borrow_depths: HashMap<LocalId, u32> = HashMap::new();
  let mut optimized = Vec::with_capacity(block.statements.len());
  let mut removed = 0;
  for statement in block.statements.drain(..)
  {
    match statement
    {
      Statement::BorrowShared { local, .. } | Statement::BorrowMutable { local, .. } =>
      {
        *borrow_depths.entry(local).or_default() += 1;
        optimized.push(statement);
      }
      Statement::EndBorrow { local, .. } =>
      {
        let depth = borrow_depths.entry(local).or_default();
        if *depth == 0
        {
          removed += 1;
        }
        else
        {
          *depth -= 1;
          optimized.push(statement);
        }
      }
      _ => optimized.push(statement),
    }
  }
  block.statements = optimized;
  removed
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::hir::async_check::Span;
  use crate::mir::{BasicBlock, BlockId, Function, Local, Terminator};

  fn span(start: u32, end: u32) -> Span
  {
    Span::new(1, start, end)
  }

  fn local(id: u32) -> Local
  {
    Local { id: LocalId(id),
            name: format!("local{id}"),
            value_type: None,
            mutable: true,
            span: span(0, 1) }
  }

  #[test]
  fn removes_unreachable_blocks()
  {
    let function = Function { name: "main".to_string(),
                              locals: vec![],
                              blocks: vec![BasicBlock { id: BlockId(0),
                                                        statements: vec![],
                                                        terminator: Some(Terminator::Goto(BlockId(1))),
                                                        span: span(0, 1) },
                                           BasicBlock { id: BlockId(1),
                                                        statements: vec![],
                                                        terminator: Some(Terminator::Return(None)),
                                                        span: span(1, 2) },
                                           BasicBlock { id: BlockId(2),
                                                        statements: vec![],
                                                        terminator: Some(Terminator::Return(None)),
                                                        span: span(2, 3) }] };

    let optimized = optimize_function(function);

    assert_eq!(optimized.function.blocks.len(), 2);
    assert_eq!(optimized.reports[0].pass, OptimizationPass::RemoveUnreachableBlocks);
    assert_eq!(optimized.reports[0].removed_items, 1);
  }

  #[test]
  fn removes_redundant_end_borrow()
  {
    let function =
      Function { name: "main".to_string(),
                 locals: vec![local(0)],
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::EndBorrow { local: LocalId(0),
                                                                                   span: span(1, 2) },
                                                            Statement::BorrowShared { local: LocalId(0),
                                                                                      span: span(2, 3) },
                                                            Statement::EndBorrow { local: LocalId(0),
                                                                                   span: span(3, 4) }],
                                           terminator: Some(Terminator::Return(None)),
                                           span: span(0, 4) }] };

    let optimized = optimize_function(function);

    assert_eq!(optimized.function.blocks[0].statements.len(), 2);
    assert_eq!(optimized.reports[0].pass, OptimizationPass::RemoveRedundantEndBorrow);
    assert_eq!(optimized.reports[0].removed_items, 1);
  }

  #[test]
  fn verified_optimizer_rejects_invalid_input()
  {
    let function = Function { name: "bad".to_string(),
                              locals: vec![],
                              blocks: vec![BasicBlock { id: BlockId(0),
                                                        statements: vec![Statement::Use { local: LocalId(9),
                                                                                          span: span(1, 2) }],
                                                        terminator: Some(Terminator::Return(None)),
                                                        span: span(0, 3) }] };

    let error = optimize_verified_function(function).expect_err("invalid input must fail");

    assert!(matches!(error, OptimizationError::InputInvalid(_)));
  }

  #[test]
  fn verified_optimizer_returns_valid_output()
  {
    let function =
      Function { name: "main".to_string(),
                 locals: vec![local(0)],
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::BorrowShared { local: LocalId(0),
                                                                                      span: span(1, 2) },
                                                            Statement::EndBorrow { local: LocalId(0),
                                                                                   span: span(2, 3) }],
                                           terminator: Some(Terminator::Return(None)),
                                           span: span(0, 4) }] };

    let optimized = optimize_verified_function(function).expect("valid input should optimize");

    assert!(verify_function(&optimized.function).is_empty());
    assert!(optimized.reports.is_empty());
  }
}
