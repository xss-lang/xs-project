/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::{HashMap, HashSet};

use super::declarations::{NominalKind, NominalType, TypeRef};
use crate::xlil::Type;

#[derive(Clone, Debug, Eq, PartialEq)]
pub(crate) struct AggregateLayout
{
  pub name: String,
  pub value_type: Type,
  pub fields: Vec<Type>,
}

#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub(crate) struct AggregateRegistry
{
  pub layouts: Vec<AggregateLayout>,
  pub types: HashMap<String, Type>,
}

pub(crate) fn build(declarations: &[NominalType]) -> Option<AggregateRegistry>
{
  let definitions = declarations.iter()
                                .filter(|declaration| declaration.kind == NominalKind::Data)
                                .map(|declaration| (declaration.name.as_str(), declaration))
                                .collect::<HashMap<_, _>>();
  let mut registry = AggregateRegistry::default();
  let mut visiting = HashSet::new();
  for declaration in declarations.iter()
                                 .filter(|declaration| declaration.kind == NominalKind::Data)
  {
    visit(declaration, &definitions, &mut visiting, &mut registry)?;
  }
  Some(registry)
}

fn visit(declaration: &NominalType,
         definitions: &HashMap<&str, &NominalType>,
         visiting: &mut HashSet<String>,
         registry: &mut AggregateRegistry)
         -> Option<Type>
{
  if let Some(value_type) = registry.types.get(&declaration.name)
  {
    return Some(*value_type);
  }
  if !visiting.insert(declaration.name.clone())
  {
    return None;
  }
  let mut fields = Vec::with_capacity(declaration.fields.len());
  for field in &declaration.fields
  {
    fields.push(match &field.ty
          {
            TypeRef::Primitive(primitive) => crate::hir::mir_lowering::primitive_to_xlil(*primitive)?,
            TypeRef::Named(name) => visit(definitions.get(name.as_str())?, definitions, visiting, registry)?,
            TypeRef::Unit => return None,
          });
  }
  visiting.remove(&declaration.name);
  let value_type = Type::aggregate(registry.layouts.len() as u32);
  registry.types.insert(declaration.name.clone(), value_type);
  registry.layouts.push(AggregateLayout { name: declaration.name.clone(),
                                          value_type,
                                          fields });
  Some(value_type)
}
