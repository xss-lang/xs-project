/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::async_check::Span;
use super::match_model::{MatchArm, MatchPattern};

use binary_type::is_supported_integer;

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
  Char(u16),
  String(String),
  None,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum BinaryOperator
{
  Add,
  Sub,
  Mul,
  Div,
  Rem,
  BitAnd,
  BitOr,
  BitXor,
  ShiftLeft,
  ShiftRight,
  Equal,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum UnaryOperator
{
  LogicalNot,
  Positive,
  Negative,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum UpdateOperator
{
  Increment,
  Decrement,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum UpdatePosition
{
  Prefix,
  Postfix,
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
  Update
  {
    target: String,
    operator: UpdateOperator,
    position: UpdatePosition,
    span: Span,
  },
  Binary
  {
    operator: BinaryOperator,
    left: Box<Expression>,
    right: Box<Expression>,
    span: Span,
  },
  Unary
  {
    operator: UnaryOperator,
    operand: Box<Expression>,
    span: Span,
  },
  Call
  {
    function: String,
    arguments: Vec<Expression>,
    parameter_types: Vec<Type>,
    return_type: Box<Type>,
    span: Span,
  },
  If
  {
    condition: Box<Expression>,
    then_block: Box<Block>,
    else_block: Box<Block>,
    result_type: Box<Type>,
    span: Span,
  },
  Match
  {
    selector: Box<Expression>,
    selector_type: Box<Type>,
    arms: Vec<MatchArm>,
    result_type: Box<Type>,
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
  If
  {
    condition: Expression,
    then_block: Block,
    else_block: Option<Block>,
    span: Span,
  },
  While
  {
    condition: Expression,
    body: Block,
    span: Span,
  },
  For
  {
    initializer: Option<Box<Statement>>,
    condition: Option<Expression>,
    update: Option<Expression>,
    body: Block,
    span: Span,
  },
  Match
  {
    selector: Expression,
    selector_type: Type,
    arms: Vec<MatchArm>,
    span: Span,
  },
  Break
  {
    span: Span,
  },
  Continue
  {
    span: Span,
  },
  Panic
  {
    span: Span,
  },
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Block
{
  pub statements: Vec<Statement>,
  pub tail: Option<Box<Expression>>,
  pub span: Span,
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
  DuplicateLocal,
  BinaryTypeMismatch,
  UnaryTypeMismatch,
  ResultPropagationRequiresResult,
  ResultPropagationReturnMismatch,
  ConditionTypeMismatch,
  MissingBlockValue,
  BreakOutsideLoop,
  ContinueOutsideLoop,
  MatchRequiresFinalElse,
  DuplicateMatchPattern,
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
  scope_starts: Vec<usize>,
  loop_depth: usize,
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
    self.scope_starts.push(0);
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
        let scope_start = self.scope_starts.last().copied().unwrap_or(0);
        if self.locals[scope_start..].iter()
                                     .any(|existing| existing.name == local.name)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::DuplicateLocal,
                                 message: format!("local '{}' is already declared in this scope", local.name),
                                 span: local.span });
        }
        else
        {
          self.locals.push(local.clone());
        }
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
      Statement::If { condition,
                      then_block,
                      else_block,
                      span, } =>
      {
        self.check_condition(condition, *span);
        self.check_block(then_block, None);
        if let Some(else_block) = else_block
        {
          self.check_block(else_block, None);
        }
      }
      Statement::While { condition,
                         body,
                         span, } =>
      {
        self.check_condition(condition, *span);
        self.loop_depth += 1;
        self.check_block(body, None);
        self.loop_depth -= 1;
      }
      Statement::For { initializer,
                       condition,
                       update,
                       body,
                       span, } => self.check_for_statement(initializer.as_deref(),
                                                           condition.as_ref(),
                                                           update.as_ref(),
                                                           body,
                                                           *span,
                                                           return_type),
      Statement::Match { selector,
                         selector_type,
                         arms,
                         span, } => self.check_match_statement(selector, selector_type, arms, *span),
      Statement::Break { span } if self.loop_depth == 0 =>
      {
        self.diagnostics
            .push(Diagnostic { code: DiagnosticCode::BreakOutsideLoop,
                               message: "break may only appear inside a loop".to_string(),
                               span: *span });
      }
      Statement::Continue { span } if self.loop_depth == 0 =>
      {
        self.diagnostics
            .push(Diagnostic { code: DiagnosticCode::ContinueOutsideLoop,
                               message: "continue may only appear inside a loop".to_string(),
                               span: *span });
      }
      Statement::Break { .. } | Statement::Continue { .. } =>
      {}
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
      Expression::Update { target,
                           span,
                           .. } =>
      {
        let Some(local) = self.find_local(target).cloned()
        else
        {
          self.diagnostics.push(Diagnostic { code: DiagnosticCode::UnknownLocal,
                                             message: format!("unknown local '{target}'"),
                                             span: *span });
          return;
        };
        if !local.mutable
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::ImmutableAssignment,
                                 message: format!("cannot update binding '{target}'"),
                                 span: *span });
        }
        if !matches!(local.ty, Type::Primitive(primitive) if is_supported_integer(primitive))
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::UnaryTypeMismatch,
                                 message: "increment and decrement require an integer binding".to_string(),
                                 span: *span });
        }
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
      Expression::Unary { operator,
                          operand,
                          span, } =>
      {
        self.check_expression(operand);
        if self.unary_expression_type(*operator, operand).is_none()
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::UnaryTypeMismatch,
                                 message: "unary expression operand is not valid for this operator".to_string(),
                                 span: *span });
        }
      }
      Expression::ResultPropagation { value,
                                      span, } =>
      {
        self.check_expression(value);
        self.check_result_propagation(value, *span);
      }
      Expression::Call { arguments,
                         parameter_types,
                         span,
                         .. } =>
      {
        if arguments.len() != parameter_types.len()
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "call argument count does not match its typed signature".to_string(),
                                 span: *span });
        }
        for (argument, parameter_type) in arguments.iter().zip(parameter_types)
        {
          self.check_expression_against_type(argument, parameter_type);
        }
      }
      Expression::If { condition,
                       then_block,
                       else_block,
                       result_type,
                       span, } =>
      {
        self.check_condition(condition, *span);
        self.check_block(then_block, Some(result_type));
        self.check_block(else_block, Some(result_type));
      }
      Expression::Match { selector,
                          selector_type,
                          arms,
                          result_type,
                          span, } => self.check_match_expression(selector, selector_type, arms, result_type, *span),
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
      Expression::Binary { operator,
                           left,
                           right,
                           span, } =>
      {
        self.check_expression(expression);
        if !self.binary_expression_matches_type(*operator, left, right, ty)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "expression is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::Unary { operator,
                          operand,
                          span, } =>
      {
        self.check_expression(expression);
        if !self.unary_expression_matches_type(*operator, operand, ty)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "unary expression is not assignable to the target type".to_string(),
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
      Expression::Assign { span, .. } | Expression::Update { span, .. } =>
      {
        self.check_expression(expression);
        if self.expression_type(expression).as_ref() != Some(ty)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "updated value is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
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
      Expression::Call { span, .. } =>
      {
        self.check_expression(expression);
        if self.expression_type(expression).as_ref() != Some(ty)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "call result is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::If { span, .. } =>
      {
        self.check_expression(expression);
        if self.expression_type(expression).as_ref() != Some(ty)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message: "if expression result is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::Match { span, .. } =>
      {
        self.check_expression(expression);
        if self.expression_type(expression).as_ref() != Some(ty)
        {
          self.diagnostics
              .push(Diagnostic { code: DiagnosticCode::LiteralTypeMismatch,
                                 message:
                                   "match expression result is not assignable to the target type".to_string(),
                                 span: *span });
        }
      }
      Expression::Literal { .. } =>
      {}
    }
  }

  fn check_condition(&mut self, condition: &Expression, span: Span)
  {
    self.check_expression(condition);
    if self.expression_type(condition) != Some(Type::Primitive(PrimitiveType::Bool))
    {
      self.diagnostics
          .push(Diagnostic { code: DiagnosticCode::ConditionTypeMismatch,
                             message: "if condition must have type Bool".to_string(),
                             span });
    }
  }

  fn check_block(&mut self, block: &Block, expected_tail: Option<&Type>)
  {
    let local_count = self.locals.len();
    self.scope_starts.push(local_count);
    for statement in &block.statements
    {
      self.check_statement(statement, self.return_type.clone().as_ref());
    }
    match (block.tail.as_deref(), expected_tail)
    {
      (Some(tail), Some(expected)) => self.check_expression_against_type(tail, expected),
      (None, Some(_)) => self.diagnostics
                             .push(Diagnostic { code: DiagnosticCode::MissingBlockValue,
                                                message: "if expression branch must produce a value".to_string(),
                                                span: block.span }),
      (Some(tail), None) => self.check_expression(tail),
      (None, None) =>
      {}
    }
    self.locals.truncate(local_count);
    self.scope_starts.pop();
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

  fn find_local(&self, name: &str) -> Option<&Local>
  {
    self.locals.iter().rev().find(|local| local.name == name)
  }
}

mod binary_type;
mod expression_type;
mod for_check;
mod match_check;
mod result_type;
mod type_semantics;
mod unary_type;

pub(crate) use result_type::result_type_parts;
pub use type_semantics::ValueOwnership;

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

#[must_use]
pub fn literal_matches_type(literal: &Literal, ty: &Type) -> bool
{
  if ty.is_boxed_optional_str()
  {
    return matches!(literal, Literal::None);
  }
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
mod tests;
