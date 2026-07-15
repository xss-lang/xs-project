/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn first_child_kind<'a>(tree: &'a SyntaxTree, parent: &'a SyntaxNode, kind: u32) -> Option<&'a SyntaxNode>
{
  parent.children
        .iter()
        .filter_map(|index| tree.nodes.get(*index))
        .find(|child| child.kind == kind)
}

pub(super) fn path_text(tree: &SyntaxTree, value: &SyntaxNode) -> String
{
  if value.kind == IDENTIFIER
  {
    return value.text.clone();
  }
  let path = if value.kind == PATH
  {
    value
  }
  else
  {
    first_child_kind(tree, value, PATH).unwrap_or(value)
  };
  path.children
      .iter()
      .filter_map(|index| tree.nodes.get(*index))
      .filter(|child| child.kind == IDENTIFIER)
      .map(|child| child.text.as_str())
      .collect::<Vec<_>>()
      .join("::")
}
