/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashSet;

use crate::hir::async_check::Span;

use super::{BasicBlock, BlockId, Function, LocalId, Statement, Terminator};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  DuplicateLocal,
  DuplicateBlock,
  MissingTerminator,
  UnknownLocal,
  UnknownBlock,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub message: String,
  pub span: Span,
}

#[must_use]
pub fn verify_function(function: &Function) -> Vec<Diagnostic>
{
  let mut verifier = Verifier::new(function);
  verifier.verify();
  verifier.diagnostics
}

struct Verifier<'a>
{
  function: &'a Function,
  locals: HashSet<LocalId>,
  blocks: HashSet<BlockId>,
  diagnostics: Vec<Diagnostic>,
}

impl<'a> Verifier<'a>
{
  fn new(function: &'a Function) -> Self
  {
    Self { function,
           locals: HashSet::new(),
           blocks: HashSet::new(),
           diagnostics: Vec::new() }
  }

  fn verify(&mut self)
  {
    self.index_locals();
    self.index_blocks();
    for block in &self.function.blocks
    {
      self.verify_block(block);
    }
  }

  fn index_locals(&mut self)
  {
    for local in &self.function.locals
    {
      if !self.locals.insert(local.id)
      {
        self.report(DiagnosticCode::DuplicateLocal,
                    format!("local id {} is declared more than once", local.id.0),
                    local.span);
      }
    }
  }

  fn index_blocks(&mut self)
  {
    for block in &self.function.blocks
    {
      if !self.blocks.insert(block.id)
      {
        self.report(DiagnosticCode::DuplicateBlock,
                    format!("block id {} is declared more than once", block.id.0),
                    block.span);
      }
    }
  }

  fn verify_block(&mut self, block: &BasicBlock)
  {
    for statement in &block.statements
    {
      self.verify_statement(statement);
    }
    match &block.terminator
    {
      Some(terminator) => self.verify_terminator(terminator, block.span),
      None => self.report(DiagnosticCode::MissingTerminator,
                          "MIR block is missing a terminator".to_string(),
                          block.span),
    }
  }

  fn verify_statement(&mut self, statement: &Statement)
  {
    match *statement
    {
      Statement::Use { local,
                       span, } |
      Statement::Move { local,
                        span, } |
      Statement::ConstI64 { local,
                            span,
                            .. } |
      Statement::BorrowShared { local,
                                span, } |
      Statement::BorrowMutable { local,
                                 span, } |
      Statement::EndBorrow { local,
                             span, } |
      Statement::Drop { local,
                        span, } => self.verify_local(local, span),
    }
  }

  fn verify_terminator(&mut self, terminator: &Terminator, span: Span)
  {
    match *terminator
    {
      Terminator::Return(Some(local)) => self.verify_local(local, span),
      Terminator::Goto(block) => self.verify_block_target(block, span),
      Terminator::Return(None) | Terminator::Unreachable =>
      {}
    }
  }

  fn verify_local(&mut self, local: LocalId, span: Span)
  {
    if !self.locals.contains(&local)
    {
      self.report(DiagnosticCode::UnknownLocal,
                  format!("local id {} is not declared in this function", local.0),
                  span);
    }
  }

  fn verify_block_target(&mut self, block: BlockId, span: Span)
  {
    if !self.blocks.contains(&block)
    {
      self.report(DiagnosticCode::UnknownBlock,
                  format!("block id {} is not declared in this function", block.0),
                  span);
    }
  }

  fn report(&mut self, code: DiagnosticCode, message: String, span: Span)
  {
    self.diagnostics.push(Diagnostic { code,
                                       message,
                                       span });
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::mir::{Local, LocalId};

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

  fn block(id: u32, terminator: Option<Terminator>) -> BasicBlock
  {
    BasicBlock { id: BlockId(id),
                 statements: Vec::new(),
                 terminator,
                 span: span(id, id + 1) }
  }

  #[test]
  fn accepts_valid_function()
  {
    let function = Function { name: "main".to_string(),
                              locals: vec![local(0)],
                              blocks: vec![block(0, Some(Terminator::Return(Some(LocalId(0)))))] };

    assert!(verify_function(&function).is_empty());
  }

  #[test]
  fn reports_duplicate_ids_and_missing_terminator()
  {
    let function = Function { name: "bad".to_string(),
                              locals: vec![local(0), local(0)],
                              blocks: vec![block(0, None), block(0, Some(Terminator::Return(None)))] };

    let diagnostics = verify_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::DuplicateLocal);
    assert_eq!(diagnostics[1].code, DiagnosticCode::DuplicateBlock);
    assert_eq!(diagnostics[2].code, DiagnosticCode::MissingTerminator);
  }

  #[test]
  fn reports_unknown_references()
  {
    let function = Function { name: "bad".to_string(),
                              locals: vec![local(0)],
                              blocks: vec![BasicBlock { id: BlockId(0),
                                                        statements: vec![Statement::Use { local: LocalId(9),
                                                                                          span: span(2, 3) }],
                                                        terminator: Some(Terminator::Goto(BlockId(9))),
                                                        span: span(0, 1) }] };

    let diagnostics = verify_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::UnknownLocal);
    assert_eq!(diagnostics[1].code, DiagnosticCode::UnknownBlock);
  }
}
