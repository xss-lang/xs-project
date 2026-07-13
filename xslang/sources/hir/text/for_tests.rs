/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{function_to_xhir, parse_xhir_function};
use crate::hir::type_check::Local;
use crate::hir::{Block, Expression, Function, Literal, PrimitiveType, Span, Statement, Type};

fn span() -> Span
{
  Span::new(1, 0, 1)
}

fn integer(value: &str) -> Expression
{
  Expression::Literal { literal: Literal::Integer(value.to_string()),
                        span: span() }
}

#[test]
fn roundtrips_classic_for_records_with_optional_header_fields()
{
  let function = Function { name: "loop_records".to_string(),
                            return_type: None,
                            locals: Vec::new(),
                            body: vec![Statement::For {
      initializer: Some(Box::new(Statement::Let {
        local: Local { name: "i".to_string(),
                       ty: Type::Primitive(PrimitiveType::Long),
                       mutable: true,
                       span: span() },
        initializer: Some(integer("0")),
      })),
      condition: Some(Expression::Literal { literal: Literal::Bool(true),
                                             span: span() }),
      update: Some(Expression::Assign { target: "i".to_string(),
                                        value: Box::new(integer("1")),
                                        span: span() }),
      body: Block { statements: vec![Statement::Break { span: span() }],
                    tail: None,
                    span: span() },
      span: span(),
    }] };

  let text = function_to_xhir(&function);
  let parsed = parse_xhir_function(&text).expect("for XHIR should parse");

  assert!(text.contains("for\n"));
  assert!(text.contains("initializer\n"));
  assert!(text.contains("update\n"));
  assert!(matches!(&parsed.body[0], Statement::For {
    initializer: Some(_), condition: Some(_), update: Some(_), body, ..
  } if matches!(body.statements.as_slice(), [Statement::Break { .. }])));
}
