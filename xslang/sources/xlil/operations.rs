/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum I32BinaryOperation
{
  Div,
  Rem,
  BitAnd,
  BitOr,
  BitXor,
  ShiftLeft,
  ShiftRight,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum I64BinaryOperation
{
  Div,
  Rem,
  BitAnd,
  BitOr,
  BitXor,
  ShiftLeft,
  ShiftRight,
}

impl I64BinaryOperation
{
  #[must_use]
  pub const fn text_name(self) -> &'static str
  {
    match self
    {
      Self::Div => "div.i64",
      Self::Rem => "rem.i64",
      Self::BitAnd => "and.i64",
      Self::BitOr => "or.i64",
      Self::BitXor => "xor.i64",
      Self::ShiftLeft => "shl.i64",
      Self::ShiftRight => "shr.i64",
    }
  }

  #[must_use]
  pub fn parse_text(name: &str) -> Option<Self>
  {
    Some(match name
    {
      "div.i64" => Self::Div,
      "rem.i64" => Self::Rem,
      "and.i64" => Self::BitAnd,
      "or.i64" => Self::BitOr,
      "xor.i64" => Self::BitXor,
      "shl.i64" => Self::ShiftLeft,
      "shr.i64" => Self::ShiftRight,
      _ => return None,
    })
  }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum I64ComparisonOperation
{
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
}

impl I64ComparisonOperation
{
  #[must_use]
  pub const fn text_name(self) -> &'static str
  {
    match self
    {
      Self::Less => "lt.i64",
      Self::LessEqual => "le.i64",
      Self::Greater => "gt.i64",
      Self::GreaterEqual => "ge.i64",
    }
  }

  #[must_use]
  pub fn parse_text(name: &str) -> Option<Self>
  {
    Some(match name
    {
      "lt.i64" => Self::Less,
      "le.i64" => Self::LessEqual,
      "gt.i64" => Self::Greater,
      "ge.i64" => Self::GreaterEqual,
      _ => return None,
    })
  }
}

impl I32BinaryOperation
{
  #[must_use]
  pub const fn text_name(self) -> &'static str
  {
    match self
    {
      Self::Div => "div.i32",
      Self::Rem => "rem.i32",
      Self::BitAnd => "and.i32",
      Self::BitOr => "or.i32",
      Self::BitXor => "xor.i32",
      Self::ShiftLeft => "shl.i32",
      Self::ShiftRight => "shr.i32",
    }
  }

  #[must_use]
  pub fn parse_text(name: &str) -> Option<Self>
  {
    Some(match name
    {
      "div.i32" => Self::Div,
      "rem.i32" => Self::Rem,
      "and.i32" => Self::BitAnd,
      "or.i32" => Self::BitOr,
      "xor.i32" => Self::BitXor,
      "shl.i32" => Self::ShiftLeft,
      "shr.i32" => Self::ShiftRight,
      _ => return None,
    })
  }
}

use super::{BlockId, Function, Instruction, Type, Value, ValueId};

impl Function
{
  pub fn binary_i64(&mut self,
                    block: BlockId,
                    operation: I64BinaryOperation,
                    left: ValueId,
                    right: ValueId)
                    -> Option<ValueId>
  {
    self.add_i64_operation(block,
                           left,
                           right,
                           Type::I64,
                           |result| Instruction::BinaryI64 { operation,
                                                             result,
                                                             left,
                                                             right })
  }

  pub fn compare_i64(&mut self,
                     block: BlockId,
                     operation: I64ComparisonOperation,
                     left: ValueId,
                     right: ValueId)
                     -> Option<ValueId>
  {
    self.add_i64_operation(block,
                           left,
                           right,
                           Type::BOOL,
                           |result| Instruction::CompareI64 { operation,
                                                              result,
                                                              left,
                                                              right })
  }

  fn add_i64_operation(&mut self,
                       block: BlockId,
                       left: ValueId,
                       right: ValueId,
                       result_type: Type,
                       instruction: impl FnOnce(ValueId) -> Instruction)
                       -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I64) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I64)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: result_type });
    self.block_mut(block)?.instructions.push(instruction(result));
    Some(result)
  }

  pub fn not_bool(&mut self, block: BlockId, operand: ValueId) -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(operand).is_some_and(|value| value.value_type == Type::BOOL)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::BOOL });
    self.block_mut(block)?.instructions.push(Instruction::NotBool { result,
                                                                    operand });
    Some(result)
  }
}
