/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{Parser, span};
use crate::mir::Statement;

impl Parser<'_>
{
  pub(super) fn const_f32_statement(&mut self) -> Statement
  {
    let local = self.const_i64_target();
    Statement::ConstF32 { local,
                          bits: self.float_bits(8) as u32,
                          span: span() }
  }

  pub(super) fn const_f64_statement(&mut self) -> Statement
  {
    let local = self.const_i64_target();
    Statement::ConstF64 { local,
                          bits: self.float_bits(16),
                          span: span() }
  }

  fn float_bits(&mut self, digits: usize) -> u64
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing floating-point bit pattern".to_string());
      return 0;
    };
    self.index += 1;
    let Some(value) = line.strip_prefix("bits 0x")
    else
    {
      self.report("expected hexadecimal floating-point bit pattern".to_string());
      return 0;
    };
    if value.len() != digits
    {
      self.report("floating-point bit pattern has the wrong width".to_string());
      return 0;
    }
    u64::from_str_radix(value, 16).unwrap_or_else(|_| {
                                    self.report("floating-point bit pattern is invalid".to_string());
                                    0
                                  })
  }
}
