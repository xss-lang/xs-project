/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use super::Type;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct IntegerConstant
{
  pub value_type: Type,
  pub bits: u128,
}

impl IntegerConstant
{
  #[must_use]
  pub const fn new(value_type: Type, bits: u128) -> Option<Self>
  {
    let Some(width) = value_type.integer_width()
    else
    {
      return None;
    };
    if width < 128 && bits >= (1_u128 << width)
    {
      return None;
    }
    Some(Self { value_type,
                bits })
  }

  #[must_use]
  pub fn hexadecimal_digits(self) -> usize
  {
    self.value_type.integer_width().unwrap_or(0) as usize / 4
  }
}
