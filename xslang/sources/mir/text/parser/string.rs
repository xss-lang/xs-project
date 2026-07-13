/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{Parser, span};
use crate::mir::Statement;

impl Parser<'_>
{
  pub(super) fn const_str_statement(&mut self) -> Statement
  {
    let local = self.const_i64_target();
    let line = self.current();
    let units = match line.as_deref().and_then(|value| value.strip_prefix("value "))
    {
      Some(value) =>
      {
        self.index += 1;
        crate::text_utf16::parse_units(value).unwrap_or_else(|| {
                                               self.report("invalid const.str UTF-16 code units".to_string());
                                               Vec::new()
                                             })
      }
      None =>
      {
        self.report("missing const.str value".to_string());
        Vec::new()
      }
    };
    Statement::ConstStr { local,
                          units,
                          span: span() }
  }
}
