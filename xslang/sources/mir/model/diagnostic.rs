/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use crate::hir::async_check::Span;

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  MissingTerminator,
  UnknownLocal,
  UseAfterMove,
  MoveWhileBorrowed,
  MutableBorrowConflict,
  ImmutableLocalMutableBorrow,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub message: String,
  pub span: Span,
}
