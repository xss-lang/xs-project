/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{Block, Expression, Span, Statement, Type, TypeChecker};

impl TypeChecker
{
  pub(super) fn check_for_statement(&mut self,
                                    initializer: Option<&Statement>,
                                    condition: Option<&Expression>,
                                    update: Option<&Expression>,
                                    body: &Block,
                                    span: Span,
                                    return_type: Option<&Type>)
  {
    let local_count = self.locals.len();
    self.scope_starts.push(local_count);
    if let Some(initializer) = initializer
    {
      self.check_statement(initializer, return_type);
    }
    if let Some(condition) = condition
    {
      self.check_condition(condition, span);
    }
    self.loop_depth += 1;
    self.check_block(body, None);
    self.loop_depth -= 1;
    if let Some(update) = update
    {
      self.check_expression(update);
    }
    self.locals.truncate(local_count);
    self.scope_starts.pop();
  }
}
