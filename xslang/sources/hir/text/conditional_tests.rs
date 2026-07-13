/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{function_to_xhir, parse_xhir_function};
use crate::hir::{
  Block, DiagnosticCode, Expression, Function, Literal, PrimitiveType, Span, Statement, Type, TypeChecker,
};

fn span() -> Span
{
  Span::new(1, 0, 1)
}

fn value_block(value: &str) -> Block
{
  Block { statements: Vec::new(),
          tail: Some(Box::new(Expression::Literal { literal: Literal::Integer(value.to_string()),
                                                    span: span() })),
          span: span() }
}

#[test]
fn roundtrips_typed_if_expression_with_explicit_blocks()
{
  let long = Type::Primitive(PrimitiveType::Long);
  let function = Function { name: "choose".to_string(),
                            return_type: Some(long.clone()),
                            locals: Vec::new(),
                            body: vec![Statement::Return {
      value: Some(Expression::If { condition: Box::new(Expression::Literal { literal: Literal::Bool(true),
                                                                              span: span() }),
                                   then_block: Box::new(value_block("1")),
                                   else_block: Box::new(value_block("2")),
                                   result_type: Box::new(long),
                                   span: span() }),
      span: span(),
    }] };

  let text = function_to_xhir(&function);
  let parsed = parse_xhir_function(&text).expect("conditional XHIR should parse");

  assert!(text.contains("if_expression Long"));
  assert!(text.contains("then\n"));
  assert!(text.contains("tail\n"));
  assert!(matches!(&parsed.body[0], Statement::Return { value: Some(Expression::If { .. }),
                                                        .. }));
  assert!(TypeChecker::new().check_function(&parsed).is_empty());
}

#[test]
fn rejects_non_bool_if_condition()
{
  let long = Type::Primitive(PrimitiveType::Long);
  let function = Function { name: "invalid".to_string(),
                            return_type: Some(long.clone()),
                            locals: Vec::new(),
                            body: vec![Statement::Return {
      value: Some(Expression::If { condition: Box::new(Expression::Literal { literal: Literal::Integer("1".to_string()),
                                                                              span: span() }),
                                   then_block: Box::new(value_block("1")),
                                   else_block: Box::new(value_block("2")),
                                   result_type: Box::new(long),
                                   span: span() }),
      span: span(),
    }] };

  let diagnostics = TypeChecker::new().check_function(&function);
  assert!(diagnostics.iter()
                     .any(|diagnostic| diagnostic.code == DiagnosticCode::ConditionTypeMismatch));
}
