/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use crate::hir::{declarations, type_check::Type};
use crate::mono::{GenericInstance, TypeArgument, create_mono_plan};

use super::*;

#[derive(Clone)]
pub(super) struct Template
{
  pub(super) tree: usize,
  pub(super) node: usize,
  pub(super) name: String,
  pub(super) parameters: Vec<String>,
  pub(super) signature: declarations::Function,
  pub(super) ordinal: usize,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub(super) struct Use
{
  pub(super) name: String,
  pub(super) arguments: Vec<declarations::TypeRef>,
}

pub(super) fn parameters(tree: &SyntaxTree, function: &SyntaxNode) -> Option<Vec<String>>
{
  let parameters = function.children
                           .iter()
                           .filter_map(|index| tree.nodes.get(*index))
                           .filter(|child| child.kind == GENERIC_PARAMETER)
                           .map(|parameter| first_child_kind(tree, parameter, IDENTIFIER).map(|name| name.text.clone()))
                           .collect::<Option<Vec<_>>>()?;
  (!parameters.is_empty()).then_some(parameters)
}

pub(super) fn call_use(tree: &SyntaxTree, callee: &SyntaxNode) -> Option<Use>
{
  if callee.kind != EXPR_GENERIC_QUALIFIER || callee.children.len() < 2
  {
    return None;
  }
  let base = tree.nodes.get(callee.children[0])?;
  if base.kind != PATH
  {
    return None;
  }
  let type_nodes = callee.children[1..].iter()
                                       .filter_map(|index| tree.nodes.get(*index))
                                       .collect::<Vec<_>>();
  if type_nodes.len() + 1 != callee.children.len() || type_nodes.iter().any(|argument| !is_type_node(argument.kind))
  {
    return None;
  }
  let arguments = type_nodes.iter()
                            .map(|argument| lower_type(tree, argument))
                            .collect::<Vec<_>>();
  (arguments.len() + 1 == callee.children.len()).then(|| Use { name: path_text(tree, base),
                                                               arguments })
}

pub(super) fn collect_uses(trees: &[SyntaxTree]) -> Vec<Use>
{
  let mut uses = Vec::new();
  for tree in trees
  {
    for (call_index, call) in tree.nodes.iter().enumerate().filter(|(_, node)| node.kind == EXPR_CALL)
    {
      if belongs_to_generic_template(tree, call_index)
      {
        continue;
      }
      let Some(callee) = call.children.first().and_then(|index| tree.nodes.get(*index))
      else
      {
        continue;
      };
      if let Some(call_use) = call_use(tree, callee) &&
         !uses.contains(&call_use)
      {
        uses.push(call_use);
      }
    }
  }
  uses
}

fn is_type_node(kind: u32) -> bool
{
  (TYPE_NAMED..=TYPE_UNIT).contains(&kind) || kind == TYPE_MAP
}

fn belongs_to_generic_template(tree: &SyntaxTree, mut node_index: usize) -> bool
{
  while let Some(parent) = tree.nodes.get(node_index).and_then(|node| node.parent)
  {
    let Some(node) = tree.nodes.get(parent)
    else
    {
      return false;
    };
    if node.kind == DECL_FUNCTION
    {
      return parameters(tree, node).is_some();
    }
    node_index = parent;
  }
  false
}

pub(super) fn specialize_ref(value: &declarations::TypeRef,
                             substitutions: &HashMap<String, declarations::TypeRef>)
                             -> declarations::TypeRef
{
  match value
  {
    declarations::TypeRef::Named(name) => substitutions.get(name).cloned().unwrap_or_else(|| value.clone()),
    declarations::TypeRef::Array { element,
                                   length, } =>
    {
      declarations::TypeRef::Array { element: Box::new(specialize_ref(element, substitutions)),
                                     length: *length }
    }
    declarations::TypeRef::Map { key,
                                 value, } =>
    {
      declarations::TypeRef::Map { key: Box::new(specialize_ref(key, substitutions)),
                                   value: Box::new(specialize_ref(value, substitutions)) }
    }
    declarations::TypeRef::Tuple { fields } =>
    {
      declarations::TypeRef::Tuple { fields: fields.iter()
                                                   .map(|field| {
                                                     declarations::TupleFieldRef { name: field.name.clone(),
                                                                                   ty: specialize_ref(&field.ty,
                                                                                                      substitutions) }
                                                   })
                                                   .collect() }
    }
    declarations::TypeRef::Unit | declarations::TypeRef::Primitive(_) => value.clone(),
  }
}

pub(super) fn specialization(template: &Template,
                             arguments: &[declarations::TypeRef],
                             module: &str)
                             -> Option<(declarations::Function, HashMap<String, Type>, String)>
{
  if template.parameters.len() != arguments.len()
  {
    return None;
  }
  let substitutions = template.parameters
                              .iter()
                              .cloned()
                              .zip(arguments.iter().cloned())
                              .collect::<HashMap<_, _>>();
  let mut function = template.signature.clone();
  function.name = symbol(module, &template.name, template.ordinal, arguments);
  for parameter in &mut function.parameters
  {
    parameter.ty = specialize_ref(&parameter.ty, &substitutions);
  }
  function.return_type = specialize_ref(&function.return_type, &substitutions);
  let checked = substitutions.into_iter()
                             .map(|(name, ty)| Some((name, checked_type(&ty)?)))
                             .collect::<Option<HashMap<_, _>>>()?;
  let key = call_key(&template.name,
                     &arguments.iter().map(checked_type).collect::<Option<Vec<_>>>()?);
  Some((function, checked, key))
}

pub(super) fn specialized_call_key(tree: &SyntaxTree,
                                   callee: &SyntaxNode,
                                   substitutions: &HashMap<String, Type>)
                                   -> Option<String>
{
  let call = call_use(tree, callee)?;
  let arguments = call.arguments
                      .iter()
                      .map(|argument| {
                        let mut checked = checked_type(argument)?;
                        substitute_checked_type(&mut checked, substitutions);
                        Some(checked)
                      })
                      .collect::<Option<Vec<_>>>()?;
  Some(call_key(&call.name, &arguments))
}

pub(super) fn call_key(name: &str, arguments: &[Type]) -> String
{
  let arguments = arguments.iter().map(type_key).collect::<Vec<_>>().join(",");
  format!("{name}::<{arguments}>")
}

pub(super) fn substitute_checked_type(value: &mut Type, substitutions: &HashMap<String, Type>)
{
  match value
  {
    Type::Named(name) =>
    {
      if let Some(replacement) = substitutions.get(name)
      {
        *value = replacement.clone();
      }
    }
    Type::Array { element, .. } | Type::Set { element } => substitute_checked_type(element, substitutions),
    Type::Map { key,
                value, } =>
    {
      substitute_checked_type(key, substitutions);
      substitute_checked_type(value, substitutions);
    }
    Type::Tuple { fields } =>
    {
      for field in fields
      {
        substitute_checked_type(&mut field.ty, substitutions);
      }
    }
    Type::Unit | Type::Primitive(_) =>
    {}
  }
}

pub(super) fn checked_type_in_context(value: &declarations::TypeRef, context: &LoweringContext) -> Option<Type>
{
  let mut checked = checked_type(value)?;
  substitute_checked_type(&mut checked, &context.type_substitutions);
  Some(checked)
}

fn type_key(value: &Type) -> String
{
  match value
  {
    Type::Unit => "unit".to_string(),
    Type::Primitive(value) => format!("{value:?}"),
    Type::Named(name) => name.clone(),
    Type::Array { element,
                  length, } => match length
    {
      Some(length) => format!("[{};{length}]", type_key(element)),
      None => format!("[{}]", type_key(element)),
    },
    Type::Set { element } => format!("set<{}>", type_key(element)),
    Type::Map { key,
                value, } => format!("map<{},{}>", type_key(key), type_key(value)),
    Type::Tuple { fields } => format!("({})",
                                      fields.iter()
                                            .map(|field| type_key(&field.ty))
                                            .collect::<Vec<_>>()
                                            .join(",")),
  }
}

fn symbol(module: &str, name: &str, ordinal: usize, arguments: &[declarations::TypeRef]) -> String
{
  let instance = GenericInstance { module_path: module.to_string(),
                                   function_name: format!("{name}$G{ordinal}"),
                                   type_arguments:
                                     arguments.iter()
                                              .map(|argument| TypeArgument { name: type_ref_key(argument) })
                                              .collect() };
  create_mono_plan([instance]).entries
                              .into_iter()
                              .next()
                              .map_or_else(|| format!("xs$mono${name}${ordinal}"), |entry| entry.symbol_name)
}

fn type_ref_key(value: &declarations::TypeRef) -> String
{
  match value
  {
    declarations::TypeRef::Unit => "unit".to_string(),
    declarations::TypeRef::Primitive(value) => format!("{value:?}"),
    declarations::TypeRef::Named(name) => name.clone(),
    declarations::TypeRef::Array { element,
                                   length, } => match length
    {
      Some(length) => format!("[{};{length}]", type_ref_key(element)),
      None => format!("[{}]", type_ref_key(element)),
    },
    declarations::TypeRef::Map { key,
                                 value, } => format!("[{}:{}]", type_ref_key(key), type_ref_key(value)),
    declarations::TypeRef::Tuple { fields } => format!("({})",
                                                       fields.iter()
                                                             .map(|field| type_ref_key(&field.ty))
                                                             .collect::<Vec<_>>()
                                                             .join(",")),
  }
}
