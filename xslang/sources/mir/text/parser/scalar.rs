/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{Parser, span};
use crate::mir::Statement;

impl Parser<'_>
{
  pub(super) fn const_u16_statement(&mut self) -> Statement
  {
    let local = self.const_i64_target();
    let line = self.current();
    let value = match line.as_deref().and_then(|value| value.strip_prefix("value 0x"))
    {
      Some(hexadecimal) if hexadecimal.len() == 4 =>
      {
        self.index += 1;
        u16::from_str_radix(hexadecimal, 16).unwrap_or_else(|_| {
                                              self.report("invalid const.u16 value".to_string());
                                              0
                                            })
      }
      _ =>
      {
        self.report("missing const.u16 value".to_string());
        0
      }
    };
    Statement::ConstU16 { local,
                          value,
                          span: span() }
  }
}
