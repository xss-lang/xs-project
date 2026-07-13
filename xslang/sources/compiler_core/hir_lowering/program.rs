/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

struct FunctionLocation
{
  tree: usize,
  node: usize,
  function: usize,
}

fn module_name(tree: &SyntaxTree, root: &SyntaxNode) -> Option<String>
{
  root.children
      .iter()
      .filter_map(|index| tree.nodes.get(*index))
      .find(|child| child.kind == DECL_MODULE)
      .and_then(|module| first_child_kind(tree, module, PATH))
      .map(|path| path_text(tree, path))
}

pub(super) fn lower_program(trees: &[SyntaxTree]) -> Result<declarations::Module, LoweringError>
{
  let mut program_name: Option<Option<String>> = None;
  let mut functions = Vec::new();
  let mut locations = Vec::new();
  for (tree_index, tree) in trees.iter().enumerate()
  {
    let root = node(tree, tree.root)?;
    if root.kind != FILE
    {
      return Err(LoweringError::InvalidRoot);
    }
    let current_name = module_name(tree, root);
    if let Some(expected) = &program_name
    {
      if expected != &current_name
      {
        return Err(LoweringError::ModuleMismatch);
      }
    }
    else
    {
      program_name = Some(current_name);
    }
    for node_index in root.children.iter().copied()
    {
      let Some(function_node) = tree.nodes.get(node_index).filter(|child| child.kind == DECL_FUNCTION)
      else
      {
        continue;
      };
      let function = lower_function_signature(tree, function_node)?;
      locations.push(FunctionLocation { tree: tree_index,
                                        node: node_index,
                                        function: functions.len() });
      functions.push(function);
    }
  }
  let signatures = functions.iter()
                            .filter_map(|function| {
                              let parameters = function.parameters
                                                       .iter()
                                                       .map(|parameter| checked_type(&parameter.ty))
                                                       .collect::<Option<Vec<_>>>()?;
                              let return_type = checked_type(&function.return_type)?;
                              Some((function.name.clone(),
                                    CallSignature { parameters,
                                                    return_type }))
                            })
                            .collect::<HashMap<_, _>>();
  for location in locations
  {
    let tree = &trees[location.tree];
    let syntax = &tree.nodes[location.node];
    let function = &mut functions[location.function];
    let parameters = function.parameters.clone();
    let return_type = checked_type(&function.return_type);
    function.body = lower_body(tree, syntax, &signatures, &parameters, return_type.as_ref());
  }
  Ok(declarations::Module { name: program_name.unwrap_or(None),
                            functions })
}
