/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::mir::verify::verify_function;

fn span(start: u32, end: u32) -> Span
{
  Span::new(1, start, end)
}

fn primitive(value: PrimitiveType) -> Type
{
  Type::Primitive(value)
}

#[test]
fn maps_hir_primitives_to_xlil_value_types()
{
  assert_eq!(primitive_to_xlil(PrimitiveType::Bool),
             Some(XlilType { kind: TypeKind::Bool }));
  assert_eq!(primitive_to_xlil(PrimitiveType::Long), Some(XlilType::I32));
  assert_eq!(primitive_to_xlil(PrimitiveType::Int), Some(XlilType::I64));
  assert_eq!(primitive_to_xlil(PrimitiveType::Str), None);
}

#[test]
fn rejects_str_literal_lowering_until_runtime_layout_exists()
{
  let function = Function { name: "Name".to_string(),
                            return_type: Some(primitive(PrimitiveType::Str)),
                            locals: vec![],
                            body: vec![Statement::Return { value: Some(Expression::Literal {
                                                            literal: Literal::String("xs".to_string()),
                                                            span: span(10, 14),
                                                          }),
                                                          span: span(3, 14) }] };

  let diagnostics = HirToMirLowerer::new().lower_function(&function)
                                          .expect_err("Str lowering is intentionally deferred");
  assert!(diagnostics.iter()
                     .any(|diagnostic| diagnostic.code == DiagnosticCode::UnsupportedType));
}

#[test]
fn lowers_leading_hir_locals_as_real_mir_parameters()
{
  let parameter = super::super::type_check::Local { name: "value".to_string(),
                                                    ty: primitive(PrimitiveType::Long),
                                                    mutable: false,
                                                    span: span(0, 1) };
  let function =
    Function { name: "identity".to_string(),
               return_type: Some(primitive(PrimitiveType::Long)),
               locals: vec![parameter],
               body: vec![Statement::Return { value: Some(Expression::Local { name: "value".to_string(),
                                                                              span: span(10, 15) }),
                                              span: span(3, 16) }] };

  let mir = HirToMirLowerer::new().lower_function_with_parameters(&function, 1)
                                  .expect("parameter function should lower");
  assert_eq!(mir.parameters.len(), 1);
  assert!(mir.locals.is_empty());
  assert!(matches!(mir.blocks[0].terminator,
                   Some(mir::Terminator::Return(Some(mir::LocalId(0))))));
  assert!(verify_function(&mir).is_empty());
}
