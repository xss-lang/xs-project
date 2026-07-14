/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn report(&mut self, code: DiagnosticCode, message: impl Into<String>, span: Span)
  {
    self.diagnostics.push(Diagnostic { code,
                                       message: message.into(),
                                       span });
  }
}
