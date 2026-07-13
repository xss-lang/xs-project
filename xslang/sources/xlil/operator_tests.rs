/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::Span;
use crate::mir::{self, BasicBlock, BlockId, Local, LocalId};

use super::lowering::MirToXlilLowerer;
use super::parser::parse_module;
use super::verify::verify_module;
use super::writer::module_to_string;
use super::{Function, I32BinaryOperation, Instruction, Module, Terminator, Type};

const OPERATIONS: [I32BinaryOperation; 7] = [I32BinaryOperation::Div,
                                             I32BinaryOperation::Rem,
                                             I32BinaryOperation::BitAnd,
                                             I32BinaryOperation::BitOr,
                                             I32BinaryOperation::BitXor,
                                             I32BinaryOperation::ShiftLeft,
                                             I32BinaryOperation::ShiftRight];

fn span() -> Span
{
  Span::new(1, 0, 1)
}

#[test]
fn roundtrips_and_verifies_all_i32_binary_operation_records()
{
  let mut function = Function::definition("operator_registry", Type::I32, vec![]);
  let block = function.append_block("entry");
  let left = function.add_const_i32(block, 28).expect("left value should exist");
  let right = function.add_const_i32(block, 2).expect("right value should exist");
  let mut result = left;
  for operation in OPERATIONS
  {
    result = function.binary_i32(block, left, right, operation)
                     .expect("typed i32 operation should be created");
  }
  assert!(function.set_return(block, Some(result)));
  let mut module = Module::new("operator_registry");
  module.add_function(function);
  assert!(verify_module(&module).is_empty());

  let text = module_to_string(&module);
  for operation in OPERATIONS
  {
    assert!(text.contains(operation.text_name()));
  }
  let parsed = parse_module(&text).expect("operator registry XLIL should parse");
  assert_eq!(parsed, module);
}

#[test]
fn roundtrips_mir_operations_and_lowers_them_to_xlil()
{
  let mut statements = vec![mir::Statement::ConstI32 { local: LocalId(0),
                                                       value: 28,
                                                       span: span() },
                            mir::Statement::ConstI32 { local: LocalId(1),
                                                       value: 2,
                                                       span: span() }];
  for operation in OPERATIONS
  {
    statements.push(mir::Statement::BinaryI32 { operation,
                                                result: LocalId(2),
                                                left: LocalId(0),
                                                right: LocalId(1),
                                                span: span() });
  }
  let function = mir::Function { name: "operator_registry".to_string(),
                                 parameters: vec![],
                                 return_type: Type::I32,
                                 locals: vec![Local { id: LocalId(0),
                                                      name: "left".to_string(),
                                                      value_type: Some(Type::I32),
                                                      mutable: false,
                                                      span: span() },
                                              Local { id: LocalId(1),
                                                      name: "right".to_string(),
                                                      value_type: Some(Type::I32),
                                                      mutable: false,
                                                      span: span() },
                                              Local { id: LocalId(2),
                                                      name: "result".to_string(),
                                                      value_type: Some(Type::I32),
                                                      mutable: false,
                                                      span: span() }],
                                 blocks: vec![BasicBlock { id: BlockId(0),
                                                           statements,
                                                           terminator:
                                                             Some(mir::Terminator::Return(Some(LocalId(2)))),
                                                           span: span() }] };
  assert!(crate::mir::verify::verify_function(&function).is_empty());
  let xmir = crate::mir::text::function_to_xmir(&function);
  let parsed = crate::mir::text::parse_xmir_function(&xmir).expect("operator XMIR should parse");
  assert_eq!(crate::mir::text::function_to_xmir(&parsed), xmir);

  let lowered = MirToXlilLowerer::new().lower_function(&parsed)
                                       .expect("operator MIR should lower");
  assert_eq!(lowered.blocks[0].instructions
                              .iter()
                              .filter(|instruction| matches!(instruction, Instruction::BinaryI32 { .. }))
                              .count(),
             OPERATIONS.len());
  assert!(matches!(lowered.blocks[0].terminator, Some(Terminator::Return(_))));
}
