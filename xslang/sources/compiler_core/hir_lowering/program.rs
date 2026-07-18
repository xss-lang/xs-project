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
  constructor_owner: Option<String>,
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
  let mut nominal_types = Vec::new();
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
      let Some(declaration) = tree.nodes.get(node_index)
      else
      {
        continue;
      };
      if matches!(declaration.kind, DECL_CLASS | DECL_DATA)
      {
        let nominal = nominal::lower_nominal_type(tree, declaration)?;
        if nominal.kind == declarations::NominalKind::Data
        {
          for (ordinal, (constructor_index, constructor)) in
            declaration.children
                       .iter()
                       .filter_map(|index| tree.nodes.get(*index).map(|node| (*index, node)))
                       .filter(|(_, child)| child.kind == constructor::CLASS_CONSTRUCTOR)
                       .enumerate()
          {
            let function = constructor::lower_signature(tree, constructor, &nominal.name, ordinal)?;
            locations.push(FunctionLocation { tree: tree_index,
                                              node: constructor_index,
                                              function: functions.len(),
                                              constructor_owner: Some(nominal.name.clone()) });
            functions.push(function);
          }
        }
        nominal_types.push(nominal);
        continue;
      }
      let Some(function_node) = (declaration.kind == DECL_FUNCTION).then_some(declaration)
      else
      {
        continue;
      };
      let function = lower_function_signature(tree, function_node)?;
      locations.push(FunctionLocation { tree: tree_index,
                                        node: node_index,
                                        function: functions.len(),
                                        constructor_owner: None });
      functions.push(function);
    }
  }
  let context =
    LoweringContext { calls: functions.iter()
                                      .filter_map(|function| {
                                        let parameters = function.parameters
                                                                 .iter()
                                                                 .map(|parameter| checked_type(&parameter.ty))
                                                                 .collect::<Option<Vec<_>>>()?;
                                        let return_type = checked_type(&function.return_type)?;
                                        Some((function.name.clone(),
                                              CallSignature { symbol: function.name.clone(),
                                                              parameters,
                                                              return_type }))
                                      })
                                      .collect(),
                      constructors: functions.iter()
                                             .filter(|function| function.name.starts_with("xs$ctor$"))
                                             .filter_map(|function| {
                                               let declarations::TypeRef::Named(owner) = &function.return_type
                                               else
                                               {
                                                 return None;
                                               };
                                               let parameters = function.parameters
                                                                        .iter()
                                                                        .map(|parameter| checked_type(&parameter.ty))
                                                                        .collect::<Option<Vec<_>>>()?;
                                               Some((owner.clone(),
                                                     CallSignature { symbol: function.name.clone(),
                                                                     parameters,
                                                                     return_type:
                                                                       checked_type(&function.return_type)? }))
                                             })
                                             .fold(HashMap::<String, Vec<CallSignature>>::new(),
                                                   |mut constructors, (owner, signature)| {
                                                     constructors.entry(owner).or_default().push(signature);
                                                     constructors
                                                   }),
                      nominal_types: nominal_types.iter().cloned().map(|ty| (ty.name.clone(), ty)).collect() };
  for location in locations
  {
    let tree = &trees[location.tree];
    let syntax = &tree.nodes[location.node];
    let function = &mut functions[location.function];
    let parameters = function.parameters.clone();
    let return_type = checked_type(&function.return_type);
    function.body = if let Some(owner) = &location.constructor_owner
    {
      constructor::lower_body(tree, syntax, &context, &parameters, owner)
    }
    else
    {
      lower_body(tree, syntax, &context, &parameters, return_type.as_ref())
    };
  }
  Ok(declarations::Module { name: program_name.unwrap_or(None),
                            nominal_types,
                            functions })
}
