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
  EmptyParameterName,
  DuplicateParameter,
  DuplicateLocal,
  DuplicateBlock,
  MissingTerminator,
  UnknownLocal,
  UnknownBlock,
  MissingLocalType,
  LocalTypeMismatch,
  ReturnTypeMismatch,
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
  parameters: HashSet<String>,
  locals: HashSet<LocalId>,
  blocks: HashSet<BlockId>,
  diagnostics: Vec<Diagnostic>,
}

impl<'a> Verifier<'a>
{
  fn new(function: &'a Function) -> Self
  {
    Self { function,
           parameters: HashSet::new(),
           locals: HashSet::new(),
           blocks: HashSet::new(),
           diagnostics: Vec::new() }
  }

  fn verify(&mut self)
  {
    self.index_parameters();
    self.index_locals();
    self.index_blocks();
    for block in &self.function.blocks
    {
      self.verify_block(block);
    }
  }

  fn index_parameters(&mut self)
  {
    for parameter in &self.function.parameters
    {
      if parameter.name.is_empty()
      {
        self.report(DiagnosticCode::EmptyParameterName,
                    "MIR parameter name must not be empty".to_string(),
                    parameter.span);
      }
      if !self.parameters.insert(parameter.name.clone())
      {
        self.report(DiagnosticCode::DuplicateParameter,
                    format!("parameter '{}' is declared more than once", parameter.name),
                    parameter.span);
      }
      if !self.locals.insert(parameter.local)
      {
        self.report(DiagnosticCode::DuplicateLocal,
                    format!("local id {} is declared more than once", parameter.local.0),
                    parameter.span);
      }
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
      Statement::BorrowShared { local,
                                span, } |
      Statement::BorrowMutable { local,
                                 span, } |
      Statement::EndBorrow { local,
                             span, } |
      Statement::Drop { local,
                        span, } => self.verify_local(local, span),
      Statement::ConstI64 { local,
                            span,
                            .. } => self.verify_const_i64(local, span),
      Statement::ConstBool { local,
                             span,
                             .. } => self.verify_const_bool(local, span),
      Statement::AddI64 { result,
                          left,
                          right,
                          span, } => self.verify_i64_binary(result, left, right, "add.i64", span),
      Statement::SubI64 { result,
                          left,
                          right,
                          span, } => self.verify_i64_binary(result, left, right, "sub.i64", span),
      Statement::MulI64 { result,
                          left,
                          right,
                          span, } => self.verify_i64_binary(result, left, right, "mul.i64", span),
      Statement::EqI64 { result,
                         left,
                         right,
                         span, } => self.verify_eq_i64(result, left, right, span),
      Statement::Call { result,
                        ref arguments,
                        return_type,
                        span,
                        .. } => self.verify_call(result, arguments, return_type, span),
    }
  }

  fn verify_terminator(&mut self, terminator: &Terminator, span: Span)
  {
    match *terminator
    {
      Terminator::Return(Some(local)) => self.verify_return_value(local, span),
      Terminator::Goto(block) => self.verify_block_target(block, span),
      Terminator::BranchIf { condition,
                             then_block,
                             else_block, } =>
      {
        self.verify_bool_local(condition, "branch_if condition local", span);
        self.verify_block_target(then_block, span);
        self.verify_block_target(else_block, span);
      }
      Terminator::Return(None) =>
      {
        if self.function.return_type != crate::xlil::Type::VOID
        {
          self.report(DiagnosticCode::ReturnTypeMismatch,
                      "non-void MIR function must return a typed local".to_string(),
                      span);
        }
      }
      Terminator::Unreachable =>
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

  fn verify_const_i64(&mut self, local: LocalId, span: Span)
  {
    self.verify_local(local, span);
    let Some(local) = self.function.locals.iter().find(|candidate| candidate.id == local)
    else
    {
      return;
    };
    match local.value_type
    {
      Some(crate::xlil::Type::I64) =>
      {}
      Some(_) => self.report(DiagnosticCode::LocalTypeMismatch,
                             "const.i64 target local must have XLIL i64 type".to_string(),
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "const.i64 target local has no XLIL value type".to_string(),
                          span),
    }
  }

  fn verify_const_bool(&mut self, local: LocalId, span: Span)
  {
    self.verify_local(local, span);
    let Some(local) = self.function.locals.iter().find(|candidate| candidate.id == local)
    else
    {
      return;
    };
    match local.value_type
    {
      Some(crate::xlil::Type::BOOL) =>
      {}
      Some(_) => self.report(DiagnosticCode::LocalTypeMismatch,
                             "const.bool target local must have XLIL bool type".to_string(),
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "const.bool target local has no XLIL value type".to_string(),
                          span),
    }
  }

  fn verify_i64_binary(&mut self, result: LocalId, left: LocalId, right: LocalId, instruction: &str, span: Span)
  {
    self.verify_i64_local(result, &format!("{instruction} result local"), span);
    self.verify_i64_local(left, &format!("{instruction} left operand"), span);
    self.verify_i64_local(right, &format!("{instruction} right operand"), span);
  }

  fn verify_eq_i64(&mut self, result: LocalId, left: LocalId, right: LocalId, span: Span)
  {
    self.verify_bool_local(result, "eq.i64 result local", span);
    self.verify_i64_local(left, "eq.i64 left operand", span);
    self.verify_i64_local(right, "eq.i64 right operand", span);
  }

  fn verify_bool_local(&mut self, local: LocalId, label: &str, span: Span)
  {
    self.verify_local(local, span);
    let Some(local) = self.function.locals.iter().find(|candidate| candidate.id == local)
    else
    {
      return;
    };
    match local.value_type
    {
      Some(crate::xlil::Type::BOOL) =>
      {}
      Some(_) => self.report(DiagnosticCode::LocalTypeMismatch,
                             format!("{label} must have XLIL bool type"),
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          format!("{label} has no XLIL value type"),
                          span),
    }
  }

  fn verify_i64_local(&mut self, local: LocalId, label: &str, span: Span)
  {
    self.verify_local(local, span);
    let Some(local) = self.function.locals.iter().find(|candidate| candidate.id == local)
    else
    {
      return;
    };
    match local.value_type
    {
      Some(crate::xlil::Type::I64) =>
      {}
      Some(_) => self.report(DiagnosticCode::LocalTypeMismatch,
                             format!("{label} must have XLIL i64 type"),
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          format!("{label} has no XLIL value type"),
                          span),
    }
  }

  fn verify_return_value(&mut self, local: LocalId, span: Span)
  {
    self.verify_local(local, span);
    let Some(local) = self.function.locals.iter().find(|candidate| candidate.id == local)
    else
    {
      return;
    };
    match local.value_type
    {
      Some(value_type) if value_type == self.function.return_type =>
      {}
      Some(_) => self.report(DiagnosticCode::ReturnTypeMismatch,
                             "MIR return local type does not match function return type".to_string(),
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "MIR return local has no XLIL value type".to_string(),
                          span),
    }
  }

  fn verify_call(&mut self, result: Option<LocalId>, arguments: &[LocalId], return_type: crate::xlil::Type, span: Span)
  {
    for argument in arguments
    {
      self.verify_local(*argument, span);
    }
    match (result, return_type)
    {
      (Some(result), crate::xlil::Type::VOID) =>
      {
        self.verify_local(result, span);
        self.report(DiagnosticCode::ReturnTypeMismatch,
                    "void MIR call cannot write a result local".to_string(),
                    span);
      }
      (Some(result), return_type) =>
      {
        self.verify_local(result, span);
        let Some(local) = self.function.locals.iter().find(|candidate| candidate.id == result)
        else
        {
          return;
        };
        match local.value_type
        {
          Some(value_type) if value_type == return_type =>
          {}
          Some(_) => self.report(DiagnosticCode::LocalTypeMismatch,
                                 "MIR call result local type must match call return type".to_string(),
                                 span),
          None => self.report(DiagnosticCode::MissingLocalType,
                              "MIR call result local has no XLIL value type".to_string(),
                              span),
        }
      }
      (None, return_type) if return_type != crate::xlil::Type::VOID =>
      {
        self.report(DiagnosticCode::ReturnTypeMismatch,
                    "non-void MIR call must write a result local".to_string(),
                    span);
      }
      (None, _) =>
      {}
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
  use crate::mir::{Local, LocalId, Parameter};
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

  fn typed_local(id: u32, value_type: Type) -> Local
  {
    Local { id: LocalId(id),
            name: format!("local{id}"),
            value_type: Some(value_type),
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
                              parameters: vec![Parameter { local: LocalId(0),
                                                           name: "input".to_string(),
                                                           value_type: Type::I64,
                                                           span: span(0, 1) }],
                              return_type: Type::I64,
                              locals: vec![],
                              blocks: vec![block(0, Some(Terminator::Return(Some(LocalId(0)))))] };

    assert!(verify_function(&function).is_empty());
  }

  #[test]
  fn reports_duplicate_ids_and_missing_terminator()
  {
    let function = Function { name: "bad".to_string(),
                              parameters: vec![],
                              return_type: Type::VOID,
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
                              parameters: vec![],
                              return_type: Type::VOID,
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

  #[test]
  fn reports_return_type_mismatch()
  {
    let function = Function { name: "bad_return".to_string(),
                              parameters: vec![],
                              return_type: Type::I64,
                              locals: vec![],
                              blocks: vec![block(0, Some(Terminator::Return(None)))] };

    let diagnostics = verify_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::ReturnTypeMismatch);
  }

  #[test]
  fn reports_const_i64_type_mismatch()
  {
    let function =
      Function { name: "bad_const".to_string(),
                 parameters: vec![],
                 return_type: Type::VOID,
                 locals: vec![local(0)],
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::ConstI64 { local: LocalId(0),
                                                                                  value: 1,
                                                                                  span: span(1, 2) }],
                                           terminator: Some(Terminator::Return(None)),
                                           span: span(0, 3) }] };

    let diagnostics = verify_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::MissingLocalType);
  }

  #[test]
  fn accepts_i64_add_statement()
  {
    let function =
      Function { name: "add".to_string(),
                 parameters: vec![],
                 return_type: Type::I64,
                 locals: vec![typed_local(0, Type::I64),
                              typed_local(1, Type::I64),
                              typed_local(2, Type::I64)],
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::AddI64 { result: LocalId(2),
                                                                                left: LocalId(0),
                                                                                right: LocalId(1),
                                                                                span: span(1, 2) }],
                                           terminator: Some(Terminator::Return(Some(LocalId(2)))),
                                           span: span(0, 3) }] };

    assert!(verify_function(&function).is_empty());
  }

  #[test]
  fn accepts_branch_if_with_bool_condition()
  {
    let function = Function { name: "branch".to_string(),
                              parameters: vec![],
                              return_type: Type::VOID,
                              locals: vec![typed_local(0, Type::BOOL)],
                              blocks: vec![block(0,
                                                 Some(Terminator::BranchIf { condition: LocalId(0),
                                                                             then_block: BlockId(1),
                                                                             else_block: BlockId(2) })),
                                           block(1, Some(Terminator::Return(None))),
                                           block(2, Some(Terminator::Return(None)))] };

    assert!(verify_function(&function).is_empty());
  }

  #[test]
  fn reports_branch_if_type_and_target_errors()
  {
    let function = Function { name: "bad_branch".to_string(),
                              parameters: vec![],
                              return_type: Type::VOID,
                              locals: vec![typed_local(0, Type::I64)],
                              blocks: vec![block(0,
                                                 Some(Terminator::BranchIf { condition: LocalId(0),
                                                                             then_block: BlockId(1),
                                                                             else_block: BlockId(2) }))] };

    let diagnostics = verify_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::LocalTypeMismatch);
    assert_eq!(diagnostics[1].code, DiagnosticCode::UnknownBlock);
    assert_eq!(diagnostics[2].code, DiagnosticCode::UnknownBlock);
  }

  #[test]
  fn reports_duplicate_parameters()
  {
    let parameter = Parameter { local: LocalId(0),
                                name: "value".to_string(),
                                value_type: Type::I64,
                                span: span(0, 1) };
    let function = Function { name: "bad_parameters".to_string(),
                              parameters: vec![parameter.clone(), parameter],
                              return_type: Type::VOID,
                              locals: vec![],
                              blocks: vec![block(0, Some(Terminator::Return(None)))] };

    let diagnostics = verify_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::DuplicateParameter);
  }
}
