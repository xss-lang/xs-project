/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{DiagnosticCode, Parser};
use crate::xlil::{Function, Instruction, Type, Value};

impl Parser<'_>
{
  pub(super) fn const_u16(&mut self, function: &mut Function, text: &str, line: usize) -> Option<Instruction>
  {
    let (result, value) = text.split_once(" = const.u16 ")?;
    let result = self.value_id(result.strip_suffix(":u16")?, line)?;
    let Some(hexadecimal) = value.strip_prefix("0x").filter(|value| value.len() == 4)
    else
    {
      self.report(DiagnosticCode::InvalidInteger,
                  line,
                  "XLIL const.u16 immediate must contain four hexadecimal digits");
      return None;
    };
    let Some(value) = u16::from_str_radix(hexadecimal, 16).ok()
    else
    {
      self.report(DiagnosticCode::InvalidInteger,
                  line,
                  "XLIL const.u16 immediate is invalid");
      return None;
    };
    function.values.push(Value { id: result,
                                 value_type: Type::U16 });
    Some(Instruction::ConstU16 { result,
                                 value })
  }
}
