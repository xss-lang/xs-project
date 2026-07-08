/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::async_check::Span;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PrimitiveType
{
  Bool,
  Byte,
  SByte,
  Char,
  Short,
  Long,
  Int,
  Integer,
  UShort,
  ULong,
  UInt,
  UInteger,
  SFloat,
  Float,
  Str,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Type
{
  Primitive(PrimitiveType),
  Named(String),
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Local
{
  pub name: String,
  pub ty: Type,
  pub mutable: bool,
  pub span: Span,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Literal
{
  Bool(bool),
  Integer(String),
  Float(String),
  Char(char),
  String(String),
  None,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Expression
{
  Literal
  {
    literal: Literal, span: Span
  },
  Local
  {
    name: String, span: Span
  },
  Assign
  {
    target: String,
    value: Box<Expression>,
    span: Span,
  },
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Statement
{
  Let
  {
    local: Local,
    initializer: Option<Expression>,
  },
  Expr(Expression),
  Return
  {
    value: Option<Expression>,
    span: Span,
  },
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Function
{
  pub name: String,
  pub return_type: Option<Type>,
  pub locals: Vec<Local>,
  pub body: Vec<Statement>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  LiteralTypeMismatch,
  ImmutableAssignment,
  UnknownLocal,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub message: String,
  pub span: Span,
}

#[derive(Default)]
pub struct TypeChecker
{
  locals: Vec<Local>,
  diagnostics: Vec<Diagnostic>,
}

impl TypeChecker
{
  #[must_use]
  pub fn new() -> Self
  {
    Self::default()
  }

  #[must_use]
  pub fn check_function(mut self, function: &Function) -> Vec<Diagnostic>
  {
    self.locals.extend(function.locals.iter().cloned());
    for statement in &function.body
    {
      self.check_statement(statement, function.return_type.as_ref());
    }
    self.diagnostics
  }

  fn check_statement(&mut self, statement: &Statement, return_type: Option<&Type>)
  {
    match statement
    {
      Statement::Let { local,
                       initializer, } =>
      {
        if let Some(initializer) = initializer
        {
          self.check_expression_against_type(initializer, &local.ty);
        }
        self.locals.push(local.clone());
      }
      Statement::Expr(expression) => self.check_expression(expression),
      Statement::Return { value,
                          span, } =>
      {
        if let (Some(value), Some(return_type)) = (value, return_type)
        {
          self.check_expression_against_type(value, return_type);
        }
        if value.is_none() && return_type.is_some()
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message:
                                   "return without a value is not assignable to the function return type".to_string(),
                                 span: *span });
        }
      }
    }
  }

  fn check_expression(&mut self, expression: &Expression)
  {
    if let Expression::Assign { target,
                                value,
                                span, } = expression
    {
      let Some(local) = self.find_local(target).cloned()
      else
      {
        self.diagnostics.push(Diagnostic { code: DiagnosticCode::UnknownLocal,
                                           message: format!("unknown local '{target}'"),
                                           span: *span });
        self.check_expression(value);
        return;
      };

      if !local.mutable
      {
        self.diagnostics
            .push(Diagnostic { code: DiagnosticCode::ImmutableAssignment,
                               message: format!("cannot assign to immutable local '{target}'"),
                               span: *span });
      }
      self.check_expression_against_type(value, &local.ty);
    }
  }

  fn check_expression_against_type(&mut self, expression: &Expression, ty: &Type)
  {
    match expression
    {
      Expression::Literal { literal,
                            span, }
        if !literal_matches_type(literal, ty) =>
      {
        self.diagnostics
            .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                               message: "literal is not assignable to the target type".to_string(),
                               span: *span });
      }
      Expression::Assign { .. } => self.check_expression(expression),
      Expression::Literal { .. } | Expression::Local { .. } =>
      {}
    }
  }

  fn find_local(&self, name: &str) -> Option<&Local>
  {
    self.locals.iter().rev().find(|local| local.name == name)
  }
}

#[must_use]
pub fn literal_matches_type(literal: &Literal, ty: &Type) -> bool
{
  let Type::Primitive(primitive) = ty
  else
  {
    return true;
  };

  match literal
  {
    Literal::None => true,
    Literal::Bool(_) => *primitive == PrimitiveType::Bool,
    Literal::Integer(_) => matches!(primitive,
                                    PrimitiveType::Byte |
                                    PrimitiveType::SByte |
                                    PrimitiveType::Short |
                                    PrimitiveType::Long |
                                    PrimitiveType::Int |
                                    PrimitiveType::Integer |
                                    PrimitiveType::UShort |
                                    PrimitiveType::ULong |
                                    PrimitiveType::UInt |
                                    PrimitiveType::UInteger),
    Literal::Float(_) => matches!(primitive, PrimitiveType::SFloat | PrimitiveType::Float),
    Literal::Char(_) => *primitive == PrimitiveType::Char,
    Literal::String(_) => *primitive == PrimitiveType::Str,
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  fn span(start: u32, end: u32) -> Span
  {
    Span::new(1, start, end)
  }

  fn primitive(primitive: PrimitiveType) -> Type
  {
    Type::Primitive(primitive)
  }

  fn local(name: &str, ty: Type, mutable: bool) -> Local
  {
    Local { name: name.to_string(),
            ty,
            mutable,
            span: span(0, 1) }
  }

  #[test]
  fn validates_literal_initializer_types()
  {
    let function = Function { name: "main".to_string(),
                              return_type: None,
                              locals: vec![],
                              body: vec![
                Statement::Let {
                    local: local("count", primitive(PrimitiveType::Int), true),
                    initializer: Some(Expression::Literal {
                        literal: Literal::Integer("1".to_string()),
                        span: span(10, 11),
                    }),
                },
                Statement::Let {
                    local: local("name", primitive(PrimitiveType::Str), false),
                    initializer: Some(Expression::Literal {
                        literal: Literal::String("xs".to_string()),
                        span: span(20, 24),
                    }),
                },
            ] };

    assert!(TypeChecker::new().check_function(&function).is_empty());
  }

  #[test]
  fn rejects_literal_initializer_type_mismatch()
  {
    let function = Function { name: "main".to_string(),
                              return_type: None,
                              locals: vec![],
                              body:
                                vec![Statement::Let { local: local("count", primitive(PrimitiveType::Int), true),
                                                      initializer:
                                                        Some(Expression::Literal { literal:
                                                                                     Literal::String("bad".to_string()),
                                                                                   span: span(10, 15) }) }] };

    let diagnostics = TypeChecker::new().check_function(&function);

    assert_eq!(diagnostics.len(), 1);
    assert_eq!(diagnostics[0].code, DiagnosticCode::LiteralTypeMismatch);
  }

  #[test]
  fn rejects_assignment_to_immutable_local()
  {
    let function = Function { name: "main".to_string(),
                              return_type: None,
                              locals: vec![local("version", primitive(PrimitiveType::Str), false)],
                              body: vec![Statement::Expr(Expression::Assign {
                target: "version".to_string(),
                value: Box::new(Expression::Literal {
                    literal: Literal::String("2.0".to_string()),
                    span: span(20, 25),
                }),
                span: span(10, 25),
            })] };

    let diagnostics = TypeChecker::new().check_function(&function);

    assert_eq!(diagnostics.len(), 1);
    assert_eq!(diagnostics[0].code, DiagnosticCode::ImmutableAssignment);
  }

  #[test]
  fn validates_return_literal_type()
  {
    let function = Function { name: "answer".to_string(),
                              return_type: Some(primitive(PrimitiveType::Int)),
                              locals: vec![],
                              body: vec![Statement::Return { value: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("42".to_string()),
                                                                                  span: span(10, 12) }),
                                                span: span(3, 12) }] };

    assert!(TypeChecker::new().check_function(&function).is_empty());
  }

  #[test]
  fn rejects_return_literal_type_mismatch()
  {
    let function = Function { name: "answer".to_string(),
                              return_type: Some(primitive(PrimitiveType::Bool)),
                              locals: vec![],
                              body: vec![Statement::Return { value: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("42".to_string()),
                                                                                  span: span(10, 12) }),
                                                span: span(3, 12) }] };

    let diagnostics = TypeChecker::new().check_function(&function);

    assert_eq!(diagnostics.len(), 1);
    assert_eq!(diagnostics[0].code, DiagnosticCode::LiteralTypeMismatch);
  }
}
