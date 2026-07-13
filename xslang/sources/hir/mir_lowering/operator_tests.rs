/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::Span;
use crate::hir::text::{function_to_xhir, parse_xhir_function};
use crate::hir::{BinaryOperator, Expression, Function, Literal, PrimitiveType, Statement, Type, TypeChecker};
use crate::mir;
use crate::xlil::{I32BinaryOperation, Instruction};

use super::HirToMirLowerer;

fn long_binary(operator: BinaryOperator) -> Function
{
  let span = Span::new(1, 0, 6);
  Function { name: "operator_value".to_string(),
             return_type: Some(Type::Primitive(PrimitiveType::Long)),
             locals: vec![],
             body: vec![Statement::Return { value:
                                                    Some(Expression::Binary {
                                                      operator,
                                                      left: Box::new(Expression::Literal {
                                                        literal: Literal::Integer("28".to_string()),
                                                        span,
                                                      }),
                                                      right: Box::new(Expression::Literal {
                                                        literal: Literal::Integer("2".to_string()),
                                                        span,
                                                      }),
                                                      span,
                                                    }),
                                            span }] }
}

#[test]
fn lowers_long_integer_operators_through_xhir_mir_and_xlil()
{
  let cases = [(BinaryOperator::Div, I32BinaryOperation::Div, "binary div"),
               (BinaryOperator::Rem, I32BinaryOperation::Rem, "binary rem"),
               (BinaryOperator::BitAnd, I32BinaryOperation::BitAnd, "binary bit_and"),
               (BinaryOperator::BitOr, I32BinaryOperation::BitOr, "binary bit_or"),
               (BinaryOperator::BitXor, I32BinaryOperation::BitXor, "binary bit_xor"),
               (BinaryOperator::ShiftLeft, I32BinaryOperation::ShiftLeft, "binary shift_left"),
               (BinaryOperator::ShiftRight, I32BinaryOperation::ShiftRight, "binary shift_right")];

  for (hir_operation, expected_operation, xhir_record) in cases
  {
    let function = long_binary(hir_operation);
    assert!(TypeChecker::new().check_function(&function).is_empty());

    let xhir = function_to_xhir(&function);
    assert!(xhir.contains(xhir_record));
    let parsed = parse_xhir_function(&xhir).expect("operator XHIR should round-trip");
    let mir = HirToMirLowerer::new().lower_function(&parsed)
                                    .expect("Long operator should lower to MIR");
    assert!(crate::mir::verify::verify_function(&mir).is_empty());
    assert!(mir.blocks[0].statements.iter().any(|statement| {
                                             matches!(statement,
               mir::Statement::BinaryI32 { operation, .. } if *operation == expected_operation)
                                           }));

    let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                             .expect("operator MIR should lower to XLIL");
    assert!(xlil.blocks[0].instructions.iter().any(|instruction| {
                                                matches!(instruction,
               Instruction::BinaryI32 { operation, .. } if *operation == expected_operation)
                                              }));
  }
}
