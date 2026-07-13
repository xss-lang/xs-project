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
  assert_eq!(primitive_to_xlil(PrimitiveType::Integer),
             Some(XlilType { kind: TypeKind::I128 }));
  assert_eq!(primitive_to_xlil(PrimitiveType::SFloat),
             Some(XlilType { kind: TypeKind::F32 }));
  assert_eq!(primitive_to_xlil(PrimitiveType::Float),
             Some(XlilType { kind: TypeKind::F64 }));
  assert_eq!(primitive_to_xlil(PrimitiveType::Str), Some(XlilType::STR));
}

#[test]
fn lowers_str_literal_through_target_independent_mir()
{
  let function = Function { name: "Name".to_string(),
                            return_type: Some(primitive(PrimitiveType::Str)),
                            locals: vec![],
                            body: vec![Statement::Return { value: Some(Expression::Literal {
                                                            literal: Literal::String("xs".to_string()),
                                                            span: span(10, 14),
                                                          }),
                                                          span: span(3, 14) }] };

  let xhir = crate::hir::text::function_to_xhir(&function);
  assert!(xhir.contains("literal string \"xs\""));
  assert!(!xhir.contains("utf16"));

  let mir = HirToMirLowerer::new().lower_function(&function)
                                  .expect("Str literal should lower");
  assert!(matches!(&mir.blocks[0].statements[0],
                   mir::Statement::ConstStr { units, .. } if units == &[0x0078, 0x0073]));
  assert!(verify_function(&mir).is_empty());

  let xmir = crate::mir::text::function_to_xmir(&mir);
  assert!(xmir.contains("value utf16 [0x0078, 0x0073]"));
  assert!(!xmir.contains("utf16le"));
  assert!(!xmir.contains("utf16be"));

  let xlil = crate::xlil::lowering::MirToXlilLowerer::new().with_utf16_encoding(crate::xlil::Utf16Encoding::BigEndian)
                                                           .lower_function(&mir)
                                                           .expect("Str MIR should lower to XLIL");
  assert!(matches!(&xlil.blocks[0].instructions[0],
                   crate::xlil::Instruction::ConstStr { encoding: crate::xlil::Utf16Encoding::BigEndian,
                                                        units,
                                                        .. } if units == &[0x0078, 0x0073]));
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

#[test]
fn lowers_typed_hir_call_through_mir_and_xlil()
{
  let long = primitive(PrimitiveType::Long);
  let function = Function { name: "main".to_string(),
                            return_type: Some(long.clone()),
                            locals: vec![],
                            body: vec![Statement::Return {
      value: Some(Expression::Call {
        function: "add".to_string(),
        arguments: vec![Expression::Literal { literal: Literal::Integer("3".to_string()),
                                              span: span(10, 11) },
                        Expression::Literal { literal: Literal::Integer("4".to_string()),
                                              span: span(13, 14) }],
        parameter_types: vec![long.clone(), long],
        return_type: Box::new(primitive(PrimitiveType::Long)),
        span: span(6, 15),
      }),
      span: span(0, 16),
    }] };

  let mir = HirToMirLowerer::new().lower_function(&function)
                                  .expect("typed call should lower");
  assert!(matches!(&mir.blocks[0].statements[2],
                   mir::Statement::Call { function, arguments, .. }
                     if function == "add" && arguments.len() == 2));
  assert!(verify_function(&mir).is_empty());

  let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                           .expect("MIR call should lower to XLIL");
  assert!(matches!(&xlil.blocks[0].instructions[2],
                   crate::xlil::Instruction::Call { function, arguments, .. }
                     if function == "add" && arguments.len() == 2));
}
