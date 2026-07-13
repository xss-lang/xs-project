/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use crate::mir::{BlockId, Function, Terminator};

pub(super) fn collapse_single_predecessor_gotos(function: &mut Function) -> usize
{
  let Some(entry) = function.blocks.first().map(|block| block.id)
  else
  {
    return 0;
  };
  let mut collapsed = 0;
  loop
  {
    let predecessors = predecessor_counts(function);
    let candidate = function.blocks.iter().enumerate().find_map(|(source_index, source)| {
                                                        let Some(Terminator::Goto(target)) = source.terminator.as_ref()
                                                        else
                                                        {
                                                          return None;
                                                        };
                                                        (*target != source.id &&
                                                         *target != entry &&
                                                         predecessors.get(target) == Some(&1)).then_some((source_index,
                                                                                                          *target))
                                                      });
    let Some((source_index, target)) = candidate
    else
    {
      break;
    };
    let Some(target_index) = function.blocks.iter().position(|block| block.id == target)
    else
    {
      break;
    };
    let target_block = function.blocks[target_index].clone();
    function.blocks[source_index].statements.extend(target_block.statements);
    function.blocks[source_index].terminator = target_block.terminator;
    function.blocks.remove(target_index);
    collapsed += 1;
  }
  collapsed
}

fn predecessor_counts(function: &Function) -> HashMap<BlockId, usize>
{
  let mut counts = HashMap::new();
  for block in &function.blocks
  {
    match block.terminator
    {
      Some(Terminator::Goto(target)) => *counts.entry(target).or_default() += 1,
      Some(Terminator::BranchIf { then_block,
                                  else_block,
                                  .. }) =>
      {
        *counts.entry(then_block).or_default() += 1;
        *counts.entry(else_block).or_default() += 1;
      }
      _ =>
      {}
    }
  }
  counts
}
