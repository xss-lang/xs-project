/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn storage_locals(function: &mir::Function) -> std::collections::HashSet<mir::LocalId>
{
  function.blocks
          .iter()
          .flat_map(|block| &block.statements)
          .filter_map(|statement| match statement
          {
            mir::Statement::StoreLocal { local, .. } | mir::Statement::LoadLocal { local, .. } => Some(*local),
            _ => None,
          })
          .collect()
}
