/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use super::async_check::Span;
use super::result_desugar::{DesugaredExpression, DesugaredFunction, DesugaredStatement};
use super::type_check::{BinaryOperator, Expression, Function, Literal, PrimitiveType, Statement, Type};
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

  pub fn lower_function(self, function: &Function) -> Result<mir::Function, Vec<Diagnostic>>
  {
    self.lower_function_with_parameters(function, 0)
  }

  pub fn lower_desugared_function(mut self, function: &DesugaredFunction) -> Result<mir::Function, Vec<Diagnostic>>
  {
    let mut body = Vec::with_capacity(function.body.len());
    for statement in &function.body
    {
      if let Some(statement) = self.surface_statement_from_desugared(statement)
      {
        body.push(statement);
      }
    }
    if !self.diagnostics.is_empty()
    {
      return Err(self.diagnostics);
    }
    let surface = Function { name: function.name.clone(),
                             return_type: function.return_type.clone(),
                             locals: function.locals.clone(),
                             body };
    self.lower_function(&surface)
  }

  fn surface_statement_from_desugared(&mut self, statement: &DesugaredStatement) -> Option<Statement>
  {
    match statement
    {
      DesugaredStatement::Let { local,
                                initializer, } =>
      {
        let initializer = initializer.as_ref()
                                     .and_then(|expression| self.surface_expression_from_desugared(expression));
        Some(Statement::Let { local: local.clone(),
                              initializer })
      }
      DesugaredStatement::Expr(expression) => self.surface_expression_from_desugared(expression).map(Statement::Expr),
      DesugaredStatement::Return { value,
                                   span, } =>
      {
        let value = value.as_ref()
                         .and_then(|expression| self.surface_expression_from_desugared(expression));
        Some(Statement::Return { value,
                                 span: *span })
      }
      DesugaredStatement::Panic { span } => Some(Statement::Panic { span: *span }),
    }
  }

  fn surface_expression_from_desugared(&mut self, expression: &DesugaredExpression) -> Option<Expression>
  {
    match expression
    {
      DesugaredExpression::Literal { literal,
                                     span, } => Some(Expression::Literal { literal: literal.clone(),
                                                                           span: *span }),
      DesugaredExpression::Local { name,
                                   span, } => Some(Expression::Local { name: name.clone(),
                                                                       span: *span }),
      DesugaredExpression::Assign { target,
                                    value,
                                    span, } =>
      {
        self.surface_expression_from_desugared(value)
            .map(|value| Expression::Assign { target: target.clone(),
                                              value: Box::new(value),
                                              span: *span })
      }
      DesugaredExpression::Binary { operator,
                                    left,
                                    right,
                                    span, } =>
      {
        let left = self.surface_expression_from_desugared(left)?;
        let right = self.surface_expression_from_desugared(right)?;
        Some(Expression::Binary { operator: *operator,
                                  left: Box::new(left),
                                  right: Box::new(right),
                                  span: *span })
      }
      DesugaredExpression::ResultMatch { span, .. } =>
      {
        self.report(DiagnosticCode::UnsupportedExpression,
                    "desugared Result match lowering awaits MIR Result control-flow support",
                    *span);
        None
      }
      DesugaredExpression::Call { function,
                                  arguments,
                                  parameter_types,
                                  return_type,
                                  span, } =>
      {
        let arguments = arguments.iter()
                                 .map(|value| self.surface_expression_from_desugared(value))
                                 .collect::<Option<Vec<_>>>()?;
        Some(Expression::Call { function: function.clone(),
                                arguments,
                                parameter_types: parameter_types.clone(),
                                return_type: return_type.clone(),
                                span: *span })
      }
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
      Statement::Panic { span } => self.lower_panic(*span, lowered),
    }
  }

  fn lower_panic(&mut self, span: Span, lowered: &mut mir::Function)
  {
    if lowered.blocks[0].terminator.is_some()
    {
      return;
    }
    lowered.blocks[0].terminator = Some(mir::Terminator::Panic);
    lowered.blocks[0].span = span;
  }

  fn lower_assignment(&mut self, target: mir::LocalId, expression: &Expression, lowered: &mut mir::Function)
  {
    match expression
    {
      Expression::Literal { literal,
                            span, } => self.lower_literal_into(target, literal, *span, lowered),
      Expression::Binary { operator,
                           left,
                           right,
                           span, } => self.lower_binary_into(target, *operator, left, right, *span, lowered),
      Expression::Call { .. } => self.lower_call_into(target, expression, lowered),
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
      Some(Expression::Binary { operator,
                                left,
                                right,
                                span, }) =>
      {
        let Some(local) = self.declare_temp(lowered.return_type, *span, lowered)
        else
        {
          return;
        };
        self.lower_binary_into(local, *operator, left, right, *span, lowered);
        mir::Terminator::Return(Some(local))
      }
      Some(expression @ Expression::Call { span, .. }) =>
      {
        let Some(local) = self.declare_temp(lowered.return_type, *span, lowered)
        else
        {
          return;
        };
        self.lower_call_into(local, expression, lowered);
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

  fn lower_expression_to_local(&mut self,
                               expression: &Expression,
                               expected_type: XlilType,
                               lowered: &mut mir::Function)
                               -> Option<mir::LocalId>
  {
    match expression
    {
      Expression::Local { name,
                          span, } =>
      {
        let Some(local) = self.locals.get(name).copied()
        else
        {
          self.report(DiagnosticCode::UnknownLocal,
                      format!("unknown HIR local '{name}'"),
                      *span);
          return None;
        };
        Some(local)
      }
      Expression::Literal { literal,
                            span, } =>
      {
        let local = self.declare_temp(expected_type, *span, lowered)?;
        self.lower_literal_into(local, literal, *span, lowered);
        Some(local)
      }
      Expression::Binary { operator,
                           left,
                           right,
                           span, } =>
      {
        let local = self.declare_temp(expected_type, *span, lowered)?;
        self.lower_binary_into(local, *operator, left, right, *span, lowered);
        Some(local)
      }
      Expression::Assign { .. } =>
      {
        self.unsupported_expression(expression);
        None
      }
      Expression::ResultPropagation { .. } =>
      {
        self.unsupported_expression(expression);
        None
      }
      Expression::Call { span, .. } =>
      {
        let local = self.declare_temp(expected_type, *span, lowered)?;
        self.lower_call_into(local, expression, lowered);
        Some(local)
      }
    }
  }

  fn lower_binary_into(&mut self,
                       target: mir::LocalId,
                       operator: BinaryOperator,
                       left: &Expression,
                       right: &Expression,
                       span: Span,
                       lowered: &mut mir::Function)
  {
    let Some(target_type) = self.local_value_type(target, lowered)
    else
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "binary expression target local has no MIR value type",
                  span);
      return;
    };
    let Some(operand_type) = self.binary_operand_type(operator, target_type, left, right, lowered)
    else
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "HIR binary expression cannot lower to MIR for these operand/result types",
                  span);
      return;
    };
    let Some(left) = self.lower_expression_to_local(left, operand_type, lowered)
    else
    {
      return;
    };
    let Some(right) = self.lower_expression_to_local(right, operand_type, lowered)
    else
    {
      return;
    };
    match (operator, target_type, operand_type)
    {
      (BinaryOperator::Add, XlilType::I32, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::AddI32 { result: target,
                                                        left,
                                                        right,
                                                        span })
      }
      (BinaryOperator::Sub, XlilType::I32, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::SubI32 { result: target,
                                                        left,
                                                        right,
                                                        span })
      }
      (BinaryOperator::Mul, XlilType::I32, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::MulI32 { result: target,
                                                        left,
                                                        right,
                                                        span })
      }
      (BinaryOperator::Equal, XlilType::BOOL, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::EqI32 { result: target,
                                                       left,
                                                       right,
                                                       span })
      }
      (BinaryOperator::Less, XlilType::BOOL, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::LtI32 { result: target,
                                                       left,
                                                       right,
                                                       span })
      }
      (BinaryOperator::LessEqual, XlilType::BOOL, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::LeI32 { result: target,
                                                       left,
                                                       right,
                                                       span })
      }
      (BinaryOperator::Greater, XlilType::BOOL, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::GtI32 { result: target,
                                                       left,
                                                       right,
                                                       span })
      }
      (BinaryOperator::GreaterEqual, XlilType::BOOL, XlilType::I32) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::GeI32 { result: target,
                                                       left,
                                                       right,
                                                       span })
      }
      (BinaryOperator::Add, XlilType::I64, XlilType::I64) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::AddI64 { result: target,
                                                        left,
                                                        right,
                                                        span })
      }
      (BinaryOperator::Sub, XlilType::I64, XlilType::I64) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::SubI64 { result: target,
                                                        left,
                                                        right,
                                                        span })
      }
      (BinaryOperator::Mul, XlilType::I64, XlilType::I64) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::MulI64 { result: target,
                                                        left,
                                                        right,
                                                        span })
      }
      (BinaryOperator::Equal, XlilType::BOOL, XlilType::I64) =>
      {
        lowered.blocks[0].statements
                         .push(mir::Statement::EqI64 { result: target,
                                                       left,
                                                       right,
                                                       span })
      }
      _ => self.report(DiagnosticCode::UnsupportedExpression,
                       "HIR binary expression has no MIR instruction for this type combination",
                       span),
    }
  }

  fn binary_operand_type(&self,
                         operator: BinaryOperator,
                         target_type: XlilType,
                         left: &Expression,
                         right: &Expression,
                         lowered: &mir::Function)
                         -> Option<XlilType>
  {
    match (operator, target_type)
    {
      (BinaryOperator::Add | BinaryOperator::Sub | BinaryOperator::Mul, XlilType::I32) => Some(XlilType::I32),
      (BinaryOperator::Add | BinaryOperator::Sub | BinaryOperator::Mul, XlilType::I64) => Some(XlilType::I64),
      (BinaryOperator::Less | BinaryOperator::LessEqual | BinaryOperator::Greater | BinaryOperator::GreaterEqual,
       XlilType::BOOL) => Some(XlilType::I32),
      (BinaryOperator::Equal, XlilType::BOOL) => self.common_operand_type(left, right, lowered),
      _ => None,
    }
  }

  fn common_operand_type(&self, left: &Expression, right: &Expression, lowered: &mir::Function) -> Option<XlilType>
  {
    let left_type = self.expression_value_type(left, lowered);
    let right_type = self.expression_value_type(right, lowered);
    match (left_type, right_type)
    {
      (Some(left_type), Some(right_type)) if left_type == right_type => Some(left_type),
      (Some(value_type), None) | (None, Some(value_type)) => Some(value_type),
      _ => None,
    }
  }

  fn expression_value_type(&self, expression: &Expression, lowered: &mir::Function) -> Option<XlilType>
  {
    match expression
    {
      Expression::Local { name, .. } =>
      {
        let local = self.locals.get(name)?;
        self.local_value_type(*local, lowered)
      }
      Expression::Binary { operator,
                           left,
                           right,
                           .. } => match operator
      {
        BinaryOperator::Add | BinaryOperator::Sub | BinaryOperator::Mul =>
        {
          self.common_operand_type(left, right, lowered)
        }
        BinaryOperator::Equal |
        BinaryOperator::Less |
        BinaryOperator::LessEqual |
        BinaryOperator::Greater |
        BinaryOperator::GreaterEqual => Some(XlilType::BOOL),
      },
      Expression::Call { return_type, .. } => match return_type.as_ref()
      {
        Type::Primitive(value) => primitive_to_xlil(*value),
        Type::Named(_) => None,
      },
      Expression::Literal { .. } | Expression::Assign { .. } | Expression::ResultPropagation { .. } => None,
    }
  }

  fn local_value_type(&self, local: mir::LocalId, lowered: &mir::Function) -> Option<XlilType>
  {
    lowered.parameters
           .iter()
           .find(|candidate| candidate.local == local)
           .map(|candidate| candidate.value_type)
           .or_else(|| {
             lowered.locals
                    .iter()
                    .find(|candidate| candidate.id == local)
                    .and_then(|candidate| candidate.value_type)
           })
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
      (Literal::Integer(value), Some(XlilType { kind: TypeKind::I32 })) =>
      {
        let Ok(value) = value.replace('\'', "").parse::<i32>()
        else
        {
          self.report(DiagnosticCode::InvalidIntegerLiteral,
                      "HIR integer literal does not fit MIR const.i32",
                      span);
          return;
        };
        lowered.blocks[0].statements
                         .push(mir::Statement::ConstI32 { local: target,
                                                          value,
                                                          span });
      }
      (Literal::Integer(_), Some(_)) => self.report(DiagnosticCode::UnsupportedType,
                                                    "only Long and Int literals can lower to MIR constants today",
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
    Expression::Literal { span, .. } |
    Expression::Local { span, .. } |
    Expression::Assign { span, .. } |
    Expression::Binary { span, .. } |
    Expression::ResultPropagation { span, .. } => *span,
    Expression::Call { span, .. } => *span,
  }
}

mod call_lowering;
#[cfg(test)]
mod desugar_tests;
mod function_lowering;
#[cfg(test)]
mod value_tests;

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
  fn lowers_panic_statement_to_mir_and_xlil()
  {
    let function = Function { name: "Stop".to_string(),
                              return_type: None,
                              locals: vec![],
                              body: vec![Statement::Panic { span: span(4, 12) }] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("panic function should lower to MIR");
    let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                             .expect("panic MIR should lower to XLIL");

    assert_eq!(mir.blocks[0].terminator, Some(mir::Terminator::Panic));
    assert_eq!(xlil.blocks[0].terminator, Some(crate::xlil::Terminator::Panic));
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
  fn lowers_long_literal_to_const_i32_and_xlil()
  {
    let function =
      Function { name: "LongAnswer".to_string(),
                 return_type: Some(primitive(PrimitiveType::Long)),
                 locals: vec![],
                 body:
                   vec![Statement::Return { value:
                                              Some(Expression::Literal { literal:
                                                                           Literal::Integer("2'000'000'000".to_string()),
                                                                         span: span(10, 23) }),
                                            span: span(3, 23) }] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("Long literal return should lower");

    assert_eq!(mir.return_type, XlilType::I32);
    assert!(matches!(mir.blocks[0].statements[0], mir::Statement::ConstI32 { value:
                                                                               2_000_000_000,
                                                                             .. }));
    assert!(verify_function(&mir).is_empty());

    let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                             .expect("Long MIR should lower to XLIL");
    assert!(matches!(xlil.blocks[0].instructions[0],
                     crate::xlil::Instruction::ConstI32 { value: 2_000_000_000,
                                                          .. }));
  }

  #[test]
  fn rejects_long_literal_outside_i32_range()
  {
    let function =
      Function { name: "TooLarge".to_string(),
                 return_type: Some(primitive(PrimitiveType::Long)),
                 locals: vec![],
                 body:
                   vec![Statement::Return { value:
                                              Some(Expression::Literal { literal:
                                                                           Literal::Integer("2'147'483'648".to_string()),
                                                                         span: span(10, 23) }),
                                            span: span(3, 23) }] };

    let diagnostics = HirToMirLowerer::new().lower_function(&function)
                                            .expect_err("out-of-range Long literal must fail");

    assert!(diagnostics.iter()
                       .any(|diagnostic| diagnostic.code == DiagnosticCode::InvalidIntegerLiteral &&
                                         diagnostic.message.contains("const.i32")));
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
  fn lowers_long_binary_add_to_mir_and_xlil_i32()
  {
    let function = Function { name: "AddLong".to_string(),
                              return_type: Some(primitive(PrimitiveType::Long)),
                              locals: vec![],
                              body:
                                vec![Statement::Let { local: local("left", primitive(PrimitiveType::Long), false),
                                          initializer: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("4".to_string()),
                                                                                  span: span(1, 2) }) },
                        Statement::Let { local: local("right", primitive(PrimitiveType::Long), false),
                                         initializer: Some(Expression::Literal { literal:
                                                                                   Literal::Integer("2".to_string()),
                                                                                 span: span(3, 4) }) },
                        Statement::Return { value:
                                              Some(Expression::Binary {
                                                operator: BinaryOperator::Add,
                                                left: Box::new(Expression::Local { name: "left".to_string(),
                                                                                  span: span(5, 9) }),
                                                right: Box::new(Expression::Local { name: "right".to_string(),
                                                                                   span: span(12, 17) }),
                                                span: span(5, 17),
                                              }),
                                            span: span(0, 17) }] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("Long add should lower");

    assert!(matches!(mir.blocks[0].statements[2], mir::Statement::AddI32 { result:
                                                                             mir::LocalId(2),
                                                                           left:
                                                                             mir::LocalId(0),
                                                                           right:
                                                                             mir::LocalId(1),
                                                                           .. }));
    assert!(verify_function(&mir).is_empty());

    let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                             .expect("Long add MIR should lower to XLIL");
    assert!(matches!(xlil.blocks[0].instructions[2], crate::xlil::Instruction::AddI32 { .. }));
  }

  #[test]
  fn lowers_long_comparison_to_mir_and_xlil_i32()
  {
    let function = Function { name: "CompareLong".to_string(),
                              return_type: Some(primitive(PrimitiveType::Bool)),
                              locals: vec![],
                              body:
                                vec![Statement::Let { local: local("left", primitive(PrimitiveType::Long), false),
                                          initializer: Some(Expression::Literal { literal:
                                                                                    Literal::Integer("4".to_string()),
                                                                                  span: span(1, 2) }) },
                        Statement::Let { local: local("right", primitive(PrimitiveType::Long), false),
                                         initializer: Some(Expression::Literal { literal:
                                                                                   Literal::Integer("7".to_string()),
                                                                                 span: span(3, 4) }) },
                        Statement::Return { value:
                                              Some(Expression::Binary {
                                                operator: BinaryOperator::Less,
                                                left: Box::new(Expression::Local { name: "left".to_string(),
                                                                                  span: span(5, 9) }),
                                                right: Box::new(Expression::Local { name: "right".to_string(),
                                                                                   span: span(12, 17) }),
                                                span: span(5, 17),
                                              }),
                                            span: span(0, 17) }] };

    let mir = HirToMirLowerer::new().lower_function(&function)
                                    .expect("Long comparison should lower");

    assert!(matches!(mir.blocks[0].statements[2], mir::Statement::LtI32 { result:
                                                                            mir::LocalId(2),
                                                                          left:
                                                                            mir::LocalId(0),
                                                                          right:
                                                                            mir::LocalId(1),
                                                                          .. }));
    assert!(verify_function(&mir).is_empty());

    let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                             .expect("Long comparison MIR should lower to XLIL");
    assert!(matches!(xlil.blocks[0].instructions[2], crate::xlil::Instruction::LtI32 { .. }));
  }
}
