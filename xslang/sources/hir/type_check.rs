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

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum BinaryOperator
{
  Add,
  Sub,
  Mul,
  Equal,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
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
  Binary
  {
    operator: BinaryOperator,
    left: Box<Expression>,
    right: Box<Expression>,
    span: Span,
  },
  ResultPropagation
  {
    value: Box<Expression>, span: Span
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
  Panic
  {
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
  BinaryTypeMismatch,
  ResultPropagationRequiresResult,
  ResultPropagationReturnMismatch,
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
  return_type: Option<Type>,
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
    self.return_type = function.return_type.clone();
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
      Statement::Panic { .. } =>
      {}
    }
  }

  fn check_expression(&mut self, expression: &Expression)
  {
    match expression
    {
      Expression::Assign { target,
                           value,
                           span, } =>
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
                                 message: format!("cannot reassign binding '{target}'"),
                                 span: *span });
        }
        self.check_expression_against_type(value, &local.ty);
      }
      Expression::Binary { operator,
                           left,
                           right,
                           span, } =>
      {
        self.check_expression(left);
        self.check_expression(right);
        if self.binary_expression_type(*operator, left, right).is_none()
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::BinaryTypeMismatch,
                                 message: "binary expression operands are not valid for this operator".to_string(),
                                 span: *span });
        }
      }
      Expression::ResultPropagation { value,
                                      span, } =>
      {
        self.check_expression(value);
        self.check_result_propagation(value, *span);
      }
      Expression::Local { name,
                          span, } =>
      {
        if self.find_local(name).is_none()
        {
          self.diagnostics.push(Diagnostic { code: DiagnosticCode::UnknownLocal,
                                             message: format!("unknown local '{name}'"),
                                             span: *span });
        }
      }
      Expression::Literal { .. } =>
      {}
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
      Expression::Binary { span, .. } =>
      {
        self.check_expression(expression);
        let Some(result_type) = self.expression_type(expression)
        else
        {
          return;
        };
        if result_type != *ty
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "expression is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::Local { name,
                          span, } =>
      {
        let Some(local) = self.find_local(name)
        else
        {
          self.diagnostics.push(Diagnostic { code: DiagnosticCode::UnknownLocal,
                                             message: format!("unknown local '{name}'"),
                                             span: *span });
          return;
        };
        if local.ty != *ty
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "local expression is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::Assign { .. } => self.check_expression(expression),
      Expression::ResultPropagation { span, .. } =>
      {
        self.check_expression(expression);
        let Some(result_type) = self.expression_type(expression)
        else
        {
          return;
        };
        if result_type != *ty
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message:
                                   "Result propagation success value is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::Literal { .. } =>
      {}
    }
  }

  fn expression_type(&self, expression: &Expression) -> Option<Type>
  {
    match expression
    {
      Expression::Literal { literal, .. } => literal_default_type(literal),
      Expression::Local { name, .. } => self.find_local(name).map(|local| local.ty.clone()),
      Expression::Assign { value, .. } => self.expression_type(value),
      Expression::Binary { operator,
                           left,
                           right,
                           .. } => self.binary_expression_type(*operator, left, right),
      Expression::ResultPropagation { value, .. } => self.result_success_type(value),
    }
  }

  fn check_result_propagation(&mut self, value: &Expression, span: Span)
  {
    let Some((_, error_type)) = self.result_parts_of_expression(value)
    else
    {
      self.diagnostics
          .push(Diagnostic { code: DiagnosticCode::ResultPropagationRequiresResult,
                             message: "Result propagation with '@' requires a Result<T, E> value".to_string(),
                             span });
      return;
    };
    let Some(return_type) = &self.return_type
    else
    {
      self.diagnostics
          .push(Diagnostic { code: DiagnosticCode::ResultPropagationReturnMismatch,
                             message:
                               "Result propagation requires the enclosing function to return Result<_, E>".to_string(),
                             span });
      return;
    };
    let Some((_, return_error_type)) = result_type_parts(return_type)
    else
    {
      self.diagnostics
          .push(Diagnostic { code: DiagnosticCode::ResultPropagationReturnMismatch,
                             message:
                               "Result propagation requires the enclosing function to return Result<_, E>".to_string(),
                             span });
      return;
    };
    if error_type != return_error_type
    {
      self.diagnostics
          .push(Diagnostic { code: DiagnosticCode::ResultPropagationReturnMismatch,
                             message: "Result propagation error type is not compatible with the function return \
                                       type"
                                            .to_string(),
                             span });
    }
  }

  fn result_success_type(&self, value: &Expression) -> Option<Type>
  {
    self.result_parts_of_expression(value)
        .map(|(success_type, _)| success_type)
  }

  fn result_parts_of_expression(&self, value: &Expression) -> Option<(Type, Type)>
  {
    let value_type = self.expression_type(value)?;
    result_type_parts(&value_type)
  }

  fn binary_expression_type(&self, operator: BinaryOperator, left: &Expression, right: &Expression) -> Option<Type>
  {
    let left_type = self.expression_type(left)?;
    let right_type = self.expression_type(right)?;
    if left_type != right_type
    {
      return None;
    }
    let Type::Primitive(primitive) = left_type
    else
    {
      return None;
    };
    match operator
    {
      BinaryOperator::Add | BinaryOperator::Sub | BinaryOperator::Mul
        if matches!(primitive, PrimitiveType::Long | PrimitiveType::Int) =>
      {
        Some(Type::Primitive(primitive))
      }
      BinaryOperator::Equal if matches!(primitive, PrimitiveType::Long | PrimitiveType::Int) =>
      {
        Some(Type::Primitive(PrimitiveType::Bool))
      }
      BinaryOperator::Less | BinaryOperator::LessEqual | BinaryOperator::Greater | BinaryOperator::GreaterEqual
        if primitive == PrimitiveType::Long =>
      {
        Some(Type::Primitive(PrimitiveType::Bool))
      }
      _ => None,
    }
  }

  fn find_local(&self, name: &str) -> Option<&Local>
  {
    self.locals.iter().rev().find(|local| local.name == name)
  }
}

fn literal_default_type(literal: &Literal) -> Option<Type>
{
  let primitive = match literal
  {
    Literal::Bool(_) => PrimitiveType::Bool,
    Literal::Integer(_) => PrimitiveType::Int,
    Literal::Float(_) => PrimitiveType::Float,
    Literal::Char(_) => PrimitiveType::Char,
    Literal::String(_) => PrimitiveType::Str,
    Literal::None => return None,
  };
  Some(Type::Primitive(primitive))
}

pub(crate) fn result_type_parts(ty: &Type) -> Option<(Type, Type)>
{
  let Type::Named(name) = ty
  else
  {
    return None;
  };
  let generic = name.strip_prefix("Result<")
                    .or_else(|| name.strip_prefix("Result.Result<"))?;
  let inner = generic.strip_suffix('>')?;
  if let Some((success, error)) = split_top_level_pair(inner)
  {
    return Some((parse_named_or_primitive(success.trim()), parse_named_or_primitive(error.trim())));
  }
  Some((parse_named_or_primitive(inner.trim()), Type::Named("Result.Error".to_string())))
}

fn split_top_level_pair(text: &str) -> Option<(&str, &str)>
{
  let mut depth = 0usize;
  for (index, ch) in text.char_indices()
  {
    match ch
    {
      '<' => depth += 1,
      '>' => depth = depth.saturating_sub(1),
      ',' if depth == 0 => return Some((&text[..index], &text[index + 1..])),
      _ =>
      {}
    }
  }
  None
}

fn parse_named_or_primitive(name: &str) -> Type
{
  let primitive = match name
  {
    "Bool" => Some(PrimitiveType::Bool),
    "Byte" => Some(PrimitiveType::Byte),
    "SByte" => Some(PrimitiveType::SByte),
    "Char" => Some(PrimitiveType::Char),
    "Short" => Some(PrimitiveType::Short),
    "Long" => Some(PrimitiveType::Long),
    "Int" => Some(PrimitiveType::Int),
    "Integer" => Some(PrimitiveType::Integer),
    "UShort" => Some(PrimitiveType::UShort),
    "ULong" => Some(PrimitiveType::ULong),
    "UInt" => Some(PrimitiveType::UInt),
    "UInteger" => Some(PrimitiveType::UInteger),
    "SFloat" => Some(PrimitiveType::SFloat),
    "Float" => Some(PrimitiveType::Float),
    "Str" => Some(PrimitiveType::Str),
    _ => None,
  };
  primitive.map_or_else(|| Type::Named(name.to_string()), Type::Primitive)
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

  #[test]
  fn validates_long_binary_expression()
  {
    let function = Function { name: "compare".to_string(),
                              return_type: Some(primitive(PrimitiveType::Bool)),
                              locals: vec![local("left", primitive(PrimitiveType::Long), false),
                                           local("right", primitive(PrimitiveType::Long), false)],
                              body: vec![Statement::Return { value:
                                                                Some(Expression::Binary {
                                                                  operator: BinaryOperator::Less,
                                                                  left: Box::new(Expression::Local {
                                                                    name: "left".to_string(),
                                                                    span: span(10, 14),
                                                                  }),
                                                                  right: Box::new(Expression::Local {
                                                                    name: "right".to_string(),
                                                                    span: span(17, 22),
                                                                  }),
                                                                  span: span(10, 22),
                                                                }),
                                                              span: span(3, 22) }] };

    assert!(TypeChecker::new().check_function(&function).is_empty());
  }

  #[test]
  fn rejects_unsupported_binary_expression()
  {
    let function = Function { name: "compare".to_string(),
                              return_type: Some(primitive(PrimitiveType::Bool)),
                              locals: vec![local("left", primitive(PrimitiveType::Int), false),
                                           local("right", primitive(PrimitiveType::Int), false)],
                              body: vec![Statement::Return { value:
                                                                Some(Expression::Binary {
                                                                  operator: BinaryOperator::Less,
                                                                  left: Box::new(Expression::Local {
                                                                    name: "left".to_string(),
                                                                    span: span(10, 14),
                                                                  }),
                                                                  right: Box::new(Expression::Local {
                                                                    name: "right".to_string(),
                                                                    span: span(17, 22),
                                                                  }),
                                                                  span: span(10, 22),
                                                                }),
                                                              span: span(3, 22) }] };

    let diagnostics = TypeChecker::new().check_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::BinaryTypeMismatch);
  }

  #[test]
  fn validates_result_propagation_success_type_and_error_return()
  {
    let function = Function { name: "main".to_string(),
                              return_type: Some(Type::Named("Result<(), Error>".to_string())),
                              locals: vec![local("work", Type::Named("Result<Int, Error>".to_string()), false)],
                              body: vec![Statement::Let {
                                local: local("value", primitive(PrimitiveType::Int), false),
                                initializer: Some(Expression::ResultPropagation {
                                  value: Box::new(Expression::Local { name: "work".to_string(),
                                                                      span: span(4, 8) }),
                                  span: span(4, 9),
                                }),
                              }] };

    assert!(TypeChecker::new().check_function(&function).is_empty());
  }

  #[test]
  fn validates_single_argument_result_as_default_error()
  {
    let function = Function { name: "main".to_string(),
                              return_type: Some(Type::Named("Result<()>".to_string())),
                              locals: vec![local("work", Type::Named("Result<Int>".to_string()), false)],
                              body: vec![Statement::Let {
                                local: local("value", primitive(PrimitiveType::Int), false),
                                initializer: Some(Expression::ResultPropagation {
                                  value: Box::new(Expression::Local { name: "work".to_string(),
                                                                      span: span(4, 8) }),
                                  span: span(4, 9),
                                }),
                              }] };

    assert!(TypeChecker::new().check_function(&function).is_empty());
  }

  #[test]
  fn rejects_result_propagation_on_non_result_value()
  {
    let function = Function { name: "main".to_string(),
                              return_type: None,
                              locals: vec![local("work", primitive(PrimitiveType::Int), false)],
                              body: vec![Statement::Expr(Expression::ResultPropagation {
                                value: Box::new(Expression::Local { name: "work".to_string(),
                                                                    span: span(4, 8) }),
                                span: span(4, 9),
                              })] };

    let diagnostics = TypeChecker::new().check_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::ResultPropagationRequiresResult);
  }

  #[test]
  fn rejects_result_propagation_error_mismatch()
  {
    let function = Function { name: "main".to_string(),
                              return_type: Some(Type::Named("Result<(), OtherError>".to_string())),
                              locals: vec![local("work", Type::Named("Result<Int, Error>".to_string()), false)],
                              body: vec![Statement::Expr(Expression::ResultPropagation {
                                value: Box::new(Expression::Local { name: "work".to_string(),
                                                                    span: span(4, 8) }),
                                span: span(4, 9),
                              })] };

    let diagnostics = TypeChecker::new().check_function(&function);

    assert_eq!(diagnostics[0].code, DiagnosticCode::ResultPropagationReturnMismatch);
  }
}
