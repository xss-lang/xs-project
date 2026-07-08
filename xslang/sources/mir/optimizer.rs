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
  FoldConstI64Add,
  FoldConstI64Sub,
  FoldConstI64Mul,
  FoldConstI64Eq,
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
  let folded_adds = fold_const_i64_adds(&mut function);
  if folded_adds != 0
  {
    reports.push(OptimizationReport { pass: OptimizationPass::FoldConstI64Add,
                                      removed_items: folded_adds });
  }
  let folded_subs = fold_const_i64_subs(&mut function);
  if folded_subs != 0
  {
    reports.push(OptimizationReport { pass: OptimizationPass::FoldConstI64Sub,
                                      removed_items: folded_subs });
  }
  let folded_muls = fold_const_i64_muls(&mut function);
  if folded_muls != 0
  {
    reports.push(OptimizationReport { pass: OptimizationPass::FoldConstI64Mul,
                                      removed_items: folded_muls });
  }
  let folded_eqs = fold_const_i64_eqs(&mut function);
  if folded_eqs != 0
  {
    reports.push(OptimizationReport { pass: OptimizationPass::FoldConstI64Eq,
                                      removed_items: folded_eqs });
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

fn fold_const_i64_adds(function: &mut Function) -> usize
{
  let mut folded = 0;
  for block in &mut function.blocks
  {
    folded += fold_const_i64_adds_in_block(block);
  }
  folded
}

fn fold_const_i64_adds_in_block(block: &mut BasicBlock) -> usize
{
  let mut constants = HashMap::new();
  let mut folded = 0;
  for statement in &mut block.statements
  {
    match statement
    {
      Statement::ConstI64 { local,
                            value,
                            .. } =>
      {
        constants.insert(*local, *value);
      }
      Statement::AddI64 { result,
                          left,
                          right,
                          span, } =>
      {
        let result = *result;
        let span = *span;
        let (Some(left), Some(right)) = (constants.get(left).copied(), constants.get(right).copied())
        else
        {
          constants.remove(&result);
          continue;
        };
        let Some(value) = left.checked_add(right)
        else
        {
          constants.remove(&result);
          continue;
        };
        *statement = Statement::ConstI64 { local: result,
                                           value,
                                           span };
        constants.insert(result, value);
        folded += 1;
      }
      Statement::Call { result, .. } =>
      {
        if let Some(result) = result
        {
          constants.remove(result);
        }
      }
      _ =>
      {}
    }
  }
  folded
}

fn fold_const_i64_subs(function: &mut Function) -> usize
{
  let mut folded = 0;
  for block in &mut function.blocks
  {
    folded += fold_const_i64_subs_in_block(block);
  }
  folded
}

fn fold_const_i64_subs_in_block(block: &mut BasicBlock) -> usize
{
  let mut constants = HashMap::new();
  let mut folded = 0;
  for statement in &mut block.statements
  {
    match statement
    {
      Statement::ConstI64 { local,
                            value,
                            .. } =>
      {
        constants.insert(*local, *value);
      }
      Statement::SubI64 { result,
                          left,
                          right,
                          span, } =>
      {
        let result = *result;
        let span = *span;
        let (Some(left), Some(right)) = (constants.get(left).copied(), constants.get(right).copied())
        else
        {
          constants.remove(&result);
          continue;
        };
        let Some(value) = left.checked_sub(right)
        else
        {
          constants.remove(&result);
          continue;
        };
        *statement = Statement::ConstI64 { local: result,
                                           value,
                                           span };
        constants.insert(result, value);
        folded += 1;
      }
      Statement::Call { result, .. } =>
      {
        if let Some(result) = result
        {
          constants.remove(result);
        }
      }
      _ =>
      {}
    }
  }
  folded
}

fn fold_const_i64_muls(function: &mut Function) -> usize
{
  let mut folded = 0;
  for block in &mut function.blocks
  {
    folded += fold_const_i64_muls_in_block(block);
  }
  folded
}

fn fold_const_i64_muls_in_block(block: &mut BasicBlock) -> usize
{
  let mut constants = HashMap::new();
  let mut folded = 0;
  for statement in &mut block.statements
  {
    match statement
    {
      Statement::ConstI64 { local,
                            value,
                            .. } =>
      {
        constants.insert(*local, *value);
      }
      Statement::MulI64 { result,
                          left,
                          right,
                          span, } =>
      {
        let result = *result;
        let span = *span;
        let (Some(left), Some(right)) = (constants.get(left).copied(), constants.get(right).copied())
        else
        {
          constants.remove(&result);
          continue;
        };
        let Some(value) = left.checked_mul(right)
        else
        {
          constants.remove(&result);
          continue;
        };
        *statement = Statement::ConstI64 { local: result,
                                           value,
                                           span };
        constants.insert(result, value);
        folded += 1;
      }
      Statement::Call { result, .. } =>
      {
        if let Some(result) = result
        {
          constants.remove(result);
        }
      }
      _ =>
      {}
    }
  }
  folded
}

fn fold_const_i64_eqs(function: &mut Function) -> usize
{
  let mut folded = 0;
  for block in &mut function.blocks
  {
    folded += fold_const_i64_eqs_in_block(block);
  }
  folded
}

fn fold_const_i64_eqs_in_block(block: &mut BasicBlock) -> usize
{
  let mut constants = HashMap::new();
  let mut folded = 0;
  for statement in &mut block.statements
  {
    match statement
    {
      Statement::ConstI64 { local,
                            value,
                            .. } =>
      {
        constants.insert(*local, *value);
      }
      Statement::EqI64 { result,
                         left,
                         right,
                         span, } =>
      {
        let result = *result;
        let span = *span;
        let (Some(left), Some(right)) = (constants.get(left).copied(), constants.get(right).copied())
        else
        {
          continue;
        };
        *statement = Statement::ConstBool { local: result,
                                            value: left == right,
                                            span };
        folded += 1;
      }
      Statement::Call { result, .. } =>
      {
        if let Some(result) = result
        {
          constants.remove(result);
        }
      }
      _ =>
      {}
    }
  }
  folded
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::hir::async_check::Span;
  use crate::mir::{BasicBlock, BlockId, Function, Local, Terminator};
  use crate::xlil::Type;

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
                              parameters: vec![],
                              return_type: Type::VOID,
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
                 parameters: vec![],
                 return_type: Type::VOID,
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
                              parameters: vec![],
                              return_type: Type::VOID,
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
                 parameters: vec![],
                 return_type: Type::VOID,
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

  #[test]
  fn folds_const_i64_adds()
  {
    let function =
      Function { name: "add".to_string(),
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
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::ConstI64 { local: LocalId(0),
                                                                                  value: 2,
                                                                                  span: span(1, 2) },
                                                            Statement::ConstI64 { local: LocalId(1),
                                                                                  value: 3,
                                                                                  span: span(2, 3) },
                                                            Statement::AddI64 { result: LocalId(2),
                                                                                left: LocalId(0),
                                                                                right: LocalId(1),
                                                                                span: span(3, 4) }],
                                           terminator: Some(Terminator::Return(Some(LocalId(2)))),
                                           span: span(0, 4) }] };

    let optimized = optimize_verified_function(function).expect("valid input should optimize");

    assert_eq!(optimized.reports[0].pass, OptimizationPass::FoldConstI64Add);
    assert!(matches!(optimized.function.blocks[0].statements[2],
                     Statement::ConstI64 { local: LocalId(2),
                                           value: 5,
                                           .. }));
  }

  #[test]
  fn folds_const_i64_subs()
  {
    let function =
      Function { name: "sub".to_string(),
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
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::ConstI64 { local: LocalId(0),
                                                                                  value: 8,
                                                                                  span: span(1, 2) },
                                                            Statement::ConstI64 { local: LocalId(1),
                                                                                  value: 3,
                                                                                  span: span(2, 3) },
                                                            Statement::SubI64 { result: LocalId(2),
                                                                                left: LocalId(0),
                                                                                right: LocalId(1),
                                                                                span: span(3, 4) }],
                                           terminator: Some(Terminator::Return(Some(LocalId(2)))),
                                           span: span(0, 4) }] };

    let optimized = optimize_verified_function(function).expect("valid input should optimize");

    assert_eq!(optimized.reports[0].pass, OptimizationPass::FoldConstI64Sub);
    assert!(matches!(optimized.function.blocks[0].statements[2],
                     Statement::ConstI64 { local: LocalId(2),
                                           value: 5,
                                           .. }));
  }

  #[test]
  fn folds_const_i64_muls()
  {
    let function =
      Function { name: "mul".to_string(),
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
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::ConstI64 { local: LocalId(0),
                                                                                  value: 6,
                                                                                  span: span(1, 2) },
                                                            Statement::ConstI64 { local: LocalId(1),
                                                                                  value: 7,
                                                                                  span: span(2, 3) },
                                                            Statement::MulI64 { result: LocalId(2),
                                                                                left: LocalId(0),
                                                                                right: LocalId(1),
                                                                                span: span(3, 4) }],
                                           terminator: Some(Terminator::Return(Some(LocalId(2)))),
                                           span: span(0, 4) }] };

    let optimized = optimize_verified_function(function).expect("valid input should optimize");

    assert_eq!(optimized.reports[0].pass, OptimizationPass::FoldConstI64Mul);
    assert!(matches!(optimized.function.blocks[0].statements[2],
                     Statement::ConstI64 { local: LocalId(2),
                                           value: 42,
                                           .. }));
  }

  #[test]
  fn folds_const_i64_eqs()
  {
    let function =
      Function { name: "eq".to_string(),
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
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::ConstI64 { local: LocalId(0),
                                                                                  value: 7,
                                                                                  span: span(1, 2) },
                                                            Statement::ConstI64 { local: LocalId(1),
                                                                                  value: 7,
                                                                                  span: span(2, 3) },
                                                            Statement::EqI64 { result: LocalId(2),
                                                                               left: LocalId(0),
                                                                               right: LocalId(1),
                                                                               span: span(3, 4) }],
                                           terminator: Some(Terminator::Return(Some(LocalId(2)))),
                                           span: span(0, 4) }] };

    let optimized = optimize_verified_function(function).expect("valid input should optimize");

    assert_eq!(optimized.reports[0].pass, OptimizationPass::FoldConstI64Eq);
    assert!(matches!(optimized.function.blocks[0].statements[2],
                     Statement::ConstBool { local: LocalId(2),
                                            value: true,
                                            .. }));
  }
}
