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
