/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::xlil::{
  FloatBinaryOperation, FloatComparisonOperation, I32BinaryOperation, I64BinaryOperation, I64ComparisonOperation,
  IntegerBinaryOperation,
};

use super::{IntegerConstant, SlotId, Type, Utf16Encoding, ValueId};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Instruction
{
  ConstI64
  {
    result: ValueId, value: i64
  },
  ConstI32
  {
    result: ValueId, value: i32
  },
  ConstU16
  {
    result: ValueId, value: u16
  },
  ConstInteger
  {
    result: ValueId, value: IntegerConstant
  },
  ConstF32
  {
    result: ValueId, bits: u32
  },
  ConstF64
  {
    result: ValueId, bits: u64
  },
  ConstStr
  {
    result: ValueId,
    encoding: Utf16Encoding,
    units: Vec<u16>,
  },
  ConstBool
  {
    result: ValueId, value: bool
  },
  BinaryInteger
  {
    operation: IntegerBinaryOperation,
    value_type: Type,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  BinaryFloat
  {
    operation: FloatBinaryOperation,
    value_type: Type,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  CompareFloat
  {
    operation: FloatComparisonOperation,
    value_type: Type,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  CompareStr
  {
    operation: crate::xlil::StrComparisonOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  AddI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  SubI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  MulI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  EqI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  BinaryI64
  {
    operation: I64BinaryOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  CompareI64
  {
    operation: I64ComparisonOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  AddI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  SubI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  MulI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  BinaryI32
  {
    operation: I32BinaryOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  EqI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  LtI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  LeI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  GtI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  GeI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  NotBool
  {
    result: ValueId, operand: ValueId
  },
  Call
  {
    result: Option<ValueId>,
    function: String,
    arguments: Vec<ValueId>,
    return_type: Type,
  },
  Load
  {
    result: ValueId, slot: SlotId
  },
  Store
  {
    slot: SlotId, value: ValueId
  },
}
