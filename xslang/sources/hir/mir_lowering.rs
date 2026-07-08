/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use super::async_check::Span;
use super::type_check::{Expression, Function, Literal, PrimitiveType, Statement, Type};
use crate::mir;
use crate::xlil::{Type as XlilType, TypeKind};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  UnsupportedType,
  UnsupportedExpression,
  UnknownLocal,
  InvalidIntegerLiteral,
  MissingReturn,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub message: String,
  pub span: Span,
}

#[derive(Default)]
pub struct HirToMirLowerer
{
  diagnostics: Vec<Diagnostic>,
  locals: HashMap<String, mir::LocalId>,
  next_local: u32,
}

impl HirToMirLowerer
{
  #[must_use]
  pub fn new() -> Self
  {
    Self::default()
  }

  pub fn lower_function(mut self, function: &Function) -> Result<mir::Function, Vec<Diagnostic>>
  {
    let return_type = self.lower_return_type(function.return_type.as_ref(), Span::new(0, 0, 0));
    let mut lowered = mir::Function { name: function.name.clone(),
                                      parameters: vec![],
                                      return_type,
                                      locals: vec![],
                                      blocks: vec![mir::BasicBlock { id: mir::BlockId(0),
                                                                     statements: vec![],
                                                                     terminator: None,
                                                                     span: Span::new(0, 0, 0) }] };

    for local in &function.locals
    {
      self.declare_local(local.name.clone(), &local.ty, local.mutable, local.span, &mut lowered);
    }
    for statement in &function.body
    {
      self.lower_statement(statement, &mut lowered);
    }
    if lowered.blocks[0].terminator.is_none()
    {
      if lowered.return_type == XlilType::VOID
      {
        lowered.blocks[0].terminator = Some(mir::Terminator::Return(None));
      }
      else
      {
        self.report(DiagnosticCode::MissingReturn,
                    "non-void HIR function did not lower to a MIR return",
                    Span::new(0, 0, 0));
      }
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

  fn lower_statement(&mut self, statement: &Statement, lowered: &mut mir::Function)
  {
    match statement
    {
      Statement::Let { local,
                       initializer, } =>
      {
        let id = self.declare_local(local.name.clone(), &local.ty, local.mutable, local.span, lowered);
        if let Some(initializer) = initializer
        {
          self.lower_assignment(id, initializer, lowered);
        }
      }
      Statement::Expr(Expression::Assign { target,
                                           value,
                                           span, }) =>
      {
        let Some(id) = self.locals.get(target).copied()
        else
        {
          self.report(DiagnosticCode::UnknownLocal,
                      format!("unknown HIR local '{target}'"),
                      *span);
          return;
        };
        self.lower_assignment(id, value, lowered);
      }
      Statement::Expr(expression) => self.unsupported_expression(expression),
      Statement::Return { value,
                          span, } => self.lower_return(value.as_ref(), *span, lowered),
    }
  }

  fn lower_assignment(&mut self, target: mir::LocalId, expression: &Expression, lowered: &mut mir::Function)
  {
    match expression
    {
      Expression::Literal { literal,
                            span, } => self.lower_literal_into(target, literal, *span, lowered),
      _ => self.unsupported_expression(expression),
    }
  }

  fn lower_return(&mut self, value: Option<&Expression>, span: Span, lowered: &mut mir::Function)
  {
    if lowered.blocks[0].terminator.is_some()
    {
      return;
    }
    let terminator = match value
    {
      None => mir::Terminator::Return(None),
      Some(Expression::Local { name,
                               span, }) =>
      {
        let Some(local) = self.locals.get(name).copied()
        else
        {
          self.report(DiagnosticCode::UnknownLocal,
                      format!("unknown HIR local '{name}'"),
                      *span);
          return;
        };
        mir::Terminator::Return(Some(local))
      }
      Some(Expression::Literal { literal,
                                 span, }) =>
      {
        let Some(local) = self.declare_temp(lowered.return_type, *span, lowered)
        else
        {
          return;
        };
        self.lower_literal_into(local, literal, *span, lowered);
        mir::Terminator::Return(Some(local))
      }
      Some(expression) =>
      {
        self.unsupported_expression(expression);
        return;
      }
    };
    lowered.blocks[0].terminator = Some(terminator);
    lowered.blocks[0].span = span;
  }

  fn lower_literal_into(&mut self, target: mir::LocalId, literal: &Literal, span: Span, lowered: &mut mir::Function)
  {
    let value_type = lowered.locals
                            .iter()
                            .find(|local| local.id == target)
                            .and_then(|local| local.value_type);
    match (literal, value_type)
    {
      (Literal::Integer(value), Some(XlilType { kind: TypeKind::I64 })) =>
      {
        let Ok(value) = value.replace('\'', "").parse::<i64>()
        else
        {
          self.report(DiagnosticCode::InvalidIntegerLiteral,
                      "HIR integer literal does not fit MIR const.i64",
                      span);
          return;
        };
        lowered.blocks[0].statements
                         .push(mir::Statement::ConstI64 { local: target,
                                                          value,
                                                          span });
      }
      (Literal::Integer(_), Some(_)) => self.report(DiagnosticCode::UnsupportedType,
                                                    "only Int literals can lower to MIR const.i64 today",
                                                    span),
      (Literal::None, _) => self.report(DiagnosticCode::UnsupportedExpression,
                                        "Optional None lowering is not implemented in MIR yet",
                                        span),
      _ => self.report(DiagnosticCode::UnsupportedExpression,
                       "HIR literal kind cannot lower to MIR yet",
                       span),
    }
  }

  fn declare_local(&mut self,
                   name: String,
                   ty: &Type,
                   mutable: bool,
                   span: Span,
                   lowered: &mut mir::Function)
                   -> mir::LocalId
  {
    if let Some(id) = self.locals.get(&name).copied()
    {
      return id;
    }
    let id = mir::LocalId(self.next_local);
    self.next_local += 1;
    let value_type = self.lower_value_type(ty, span);
    lowered.locals.push(mir::Local { id,
                                     name: name.clone(),
                                     value_type,
                                     mutable,
                                     span });
    self.locals.insert(name, id);
    id
  }

  fn declare_temp(&mut self, value_type: XlilType, span: Span, lowered: &mut mir::Function) -> Option<mir::LocalId>
  {
    if value_type == XlilType::VOID
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "void return value cannot allocate a MIR local",
                  span);
      return None;
    }
    let id = mir::LocalId(self.next_local);
    self.next_local += 1;
    lowered.locals.push(mir::Local { id,
                                     name: format!("$tmp{}", id.0),
                                     value_type: Some(value_type),
                                     mutable: false,
                                     span });
    Some(id)
  }

  fn lower_return_type(&mut self, ty: Option<&Type>, span: Span) -> XlilType
  {
    ty.and_then(|ty| self.lower_value_type(ty, span))
      .unwrap_or(XlilType::VOID)
  }

  fn lower_value_type(&mut self, ty: &Type, span: Span) -> Option<XlilType>
  {
    let Type::Primitive(primitive) = ty
    else
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "user-defined HIR type has no MIR value model yet",
                  span);
      return None;
    };
    primitive_to_xlil(*primitive).or_else(|| {
                                   self.report(DiagnosticCode::UnsupportedType,
                                               "HIR primitive has no XLIL-backed MIR value type yet",
                                               span);
                                   None
                                 })
  }

  fn unsupported_expression(&mut self, expression: &Expression)
  {
    self.report(DiagnosticCode::UnsupportedExpression,
                "HIR expression cannot lower to MIR yet",
                expression_span(expression));
  }

  fn report(&mut self, code: DiagnosticCode, message: impl Into<String>, span: Span)
  {
    self.diagnostics.push(Diagnostic { code,
                                       message: message.into(),
                                       span });
  }
}

#[must_use]
pub const fn primitive_to_xlil(primitive: PrimitiveType) -> Option<XlilType>
{
  let kind = match primitive
  {
    PrimitiveType::Bool => TypeKind::Bool,
    PrimitiveType::Byte => TypeKind::U8,
    PrimitiveType::SByte => TypeKind::I8,
    PrimitiveType::Char | PrimitiveType::UShort => TypeKind::U16,
    PrimitiveType::Short => TypeKind::I16,
    PrimitiveType::Long => TypeKind::I32,
    PrimitiveType::Int => TypeKind::I64,
    PrimitiveType::Integer => TypeKind::I128,
    PrimitiveType::ULong => TypeKind::U32,
    PrimitiveType::UInt => TypeKind::U64,
    PrimitiveType::UInteger => TypeKind::U128,
    PrimitiveType::SFloat => TypeKind::F32,
    PrimitiveType::Float => TypeKind::F64,
    PrimitiveType::Str => return None,
  };
  Some(XlilType { kind })
}

const fn expression_span(expression: &Expression) -> Span
{
  match expression
  {
    Expression::Literal { span, .. } | Expression::Local { span, .. } | Expression::Assign { span, .. } => *span,
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::mir::verify::verify_function;

  fn span(start: u32, end: u32) -> Span
  {
    Span::new(1, start, end)
  }

  fn primitive(primitive: PrimitiveType) -> Type
  {
    Type::Primitive(primitive)
  }

  fn local(name: &str, ty: Type, mutable: bool) -> super::super::type_check::Local
  {
    super::super::type_check::Local { name: name.to_string(),
                                      ty,
                                      mutable,
                                      span: span(0, 1) }
  }

  #[test]
  fn maps_hir_primitives_to_xlil_value_types()
  {
    assert_eq!(primitive_to_xlil(PrimitiveType::Bool),
               Some(XlilType { kind: TypeKind::Bool }));
    assert_eq!(primitive_to_xlil(PrimitiveType::Int), Some(XlilType::I64));
    assert_eq!(primitive_to_xlil(PrimitiveType::Str), None);
  }

  #[test]
  fn lowers_void_function_to_single_return_block()
  {
    let function = Function { name: "Main".to_string(),
                              return_type: None,
                              locals: vec![],
                              body: vec![] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("void function should lower");

    assert_eq!(mir.return_type, XlilType::VOID);
    assert_eq!(mir.blocks[0].terminator, Some(mir::Terminator::Return(None)));
    assert!(verify_function(&mir).is_empty());
  }

  #[test]
  fn lowers_int_literal_return_to_const_i64()
  {
    let function = Function { name: "Answer".to_string(),
                              return_type: Some(primitive(PrimitiveType::Int)),
                              locals: vec![],
                              body: vec![Statement::Return { value: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("42".to_string()),
                                                                                  span: span(10, 12) }),
                                                            span: span(3, 12) }] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("Int literal return should lower");

    assert_eq!(mir.return_type, XlilType::I64);
    assert_eq!(mir.locals.len(), 1);
    assert!(matches!(mir.blocks[0].statements[0], mir::Statement::ConstI64 { value: 42,
                                                                             .. }));
    assert_eq!(mir.blocks[0].terminator,
               Some(mir::Terminator::Return(Some(mir::LocalId(0)))));
    assert!(verify_function(&mir).is_empty());
  }

  #[test]
  fn lowers_int_local_initializer_and_return()
  {
    let function = Function { name: "Answer".to_string(),
                              return_type: Some(primitive(PrimitiveType::Int)),
                              locals: vec![],
                              body:
                                vec![Statement::Let { local: local("answer", primitive(PrimitiveType::Int), false),
                                             initializer:
                                               Some(Expression::Literal { literal:
                                                                            Literal::Integer("1'024".to_string()),
                                                                          span: span(10, 15) }) },
                            Statement::Return { value: Some(Expression::Local { name: "answer".to_string(),
                                                                                span: span(20, 26) }),
                                                span: span(13, 26) },] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("local Int return should lower");

    assert_eq!(mir.locals[0].name, "answer");
    assert!(matches!(mir.blocks[0].statements[0], mir::Statement::ConstI64 { value: 1024,
                                                                             .. }));
    assert_eq!(mir.blocks[0].terminator,
               Some(mir::Terminator::Return(Some(mir::LocalId(0)))));
    assert!(verify_function(&mir).is_empty());
  }

  #[test]
  fn rejects_str_literal_lowering_until_runtime_layout_exists()
  {
    let function = Function { name: "Name".to_string(),
                              return_type: Some(primitive(PrimitiveType::Str)),
                              locals: vec![],
                              body: vec![Statement::Return { value: Some(Expression::Literal { literal:
                                                                                     Literal::String("xs".to_string()),
                                                                                   span: span(10, 14) }),
                                                            span: span(3, 14) }] };

    let diagnostics = HirToMirLowerer::new().lower_function(&function)
                                            .expect_err("Str lowering is intentionally deferred");

    assert!(diagnostics.iter()
                       .any(|diagnostic| diagnostic.code == DiagnosticCode::UnsupportedType));
  }
}
