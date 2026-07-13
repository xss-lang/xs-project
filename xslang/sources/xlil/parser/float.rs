/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{DiagnosticCode, Parser};
use crate::xlil::{Function, Instruction, Type, Value, type_name};

impl Parser<'_>
{
  pub(super) fn const_float(&mut self,
                            function: &mut Function,
                            result: &str,
                            value: &str,
                            value_type: Type,
                            line: usize)
                            -> Option<Instruction>
  {
    let suffix = format!(":{}", type_name(value_type));
    let result = self.value_id(result.strip_suffix(&suffix)?, line)?;
    let digits = if value_type == Type::F32
    {
      8
    }
    else
    {
      16
    };
    let hexadecimal = value.strip_prefix("0x")?;
    if hexadecimal.len() != digits
    {
      self.report(DiagnosticCode::InvalidInstruction,
                  line,
                  "XLIL floating constant has the wrong bit width");
      return None;
    }
    let bits = u64::from_str_radix(hexadecimal, 16).ok()?;
    function.values.push(Value { id: result,
                                 value_type });
    Some(if value_type == Type::F32
         {
           Instruction::ConstF32 { result,
                                   bits: bits as u32 }
         }
         else
         {
           Instruction::ConstF64 { result,
                                   bits }
         })
  }
}
