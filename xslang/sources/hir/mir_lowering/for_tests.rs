/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::hir::type_check::Local;
use crate::mir::verify::verify_function;

fn span() -> Span
{
  Span::new(1, 0, 1)
}

fn local(name: &str) -> Local
{
  Local { name: name.to_string(),
          ty: Type::Primitive(PrimitiveType::Long),
          mutable: true,
          span: span() }
}

fn local_expression(name: &str) -> Expression
{
  Expression::Local { name: name.to_string(),
                      span: span() }
}

fn integer(value: &str) -> Expression
{
  Expression::Literal { literal: Literal::Integer(value.to_string()),
                        span: span() }
}

#[test]
fn lowers_classic_for_with_continue_through_verified_mir_and_xlil()
{
  let increment = Expression::Assign { target: "i".to_string(),
                                       value: Box::new(Expression::Binary { operator: BinaryOperator::Add,
                                                                            left:
                                                                              Box::new(local_expression("i")),
                                                                            right: Box::new(integer("1")),
                                                                            span: span() }),
                                       span: span() };
  let function = Function { name: "count".to_string(),
                            return_type: Some(Type::Primitive(PrimitiveType::Long)),
                            locals: Vec::new(),
                            body: vec![Statement::For { initializer:
                                                          Some(Box::new(Statement::Let { local: local("i"),
                                                                                         initializer:
                                                                                           Some(integer("0")) })),
                                                        condition:
                                                          Some(Expression::Binary { operator: BinaryOperator::Less,
                                                                                    left:
                                                                                      Box::new(local_expression("i")),
                                                                                    right: Box::new(integer("3")),
                                                                                    span: span() }),
                                                        update: Some(increment),
                                                        body: Block { statements:
                                                                        vec![Statement::Continue { span: span() }],
                                                                      tail: None,
                                                                      span: span() },
                                                        span: span() },
                                       Statement::Return { value: Some(integer("7")),
                                                           span: span() }] };

  let diagnostics = crate::hir::TypeChecker::new().check_function(&function);
  assert!(diagnostics.is_empty(), "{diagnostics:#?}");
  let mir = HirToMirLowerer::new().lower_function(&function)
                                  .expect("classic for should lower");
  assert_eq!(mir.blocks.len(), 5);
  assert!(matches!(mir.blocks[1].terminator, Some(mir::Terminator::BranchIf { .. })));
  assert!(matches!(mir.blocks[2].terminator,
                   Some(mir::Terminator::Goto(target)) if target == mir.blocks[3].id));
  assert!(matches!(mir.blocks[3].terminator,
                   Some(mir::Terminator::Goto(target)) if target == mir.blocks[1].id));
  assert!(matches!(mir.blocks[4].terminator, Some(mir::Terminator::Return(Some(_)))));
  assert!(verify_function(&mir).is_empty());

  let xlil = crate::xlil::lowering::MirToXlilLowerer::new().lower_function(&mir)
                                                           .expect("for MIR should lower to XLIL");
  assert!(matches!(xlil.blocks[1].terminator,
                   Some(crate::xlil::Terminator::BranchIf { .. })));
  assert!(matches!(xlil.blocks[2].terminator,
                   Some(crate::xlil::Terminator::Branch(target)) if target == xlil.blocks[3].id));
  assert!(matches!(xlil.blocks[3].terminator,
                   Some(crate::xlil::Terminator::Branch(target)) if target == xlil.blocks[1].id));
}
