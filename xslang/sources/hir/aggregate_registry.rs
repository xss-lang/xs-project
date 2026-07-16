/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::{HashMap, HashSet};

use super::declarations::{self, NominalKind, NominalType, TypeRef};
use super::type_check::{Statement, Type as HirType};
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
  pub tuples: Vec<(HirType, Type)>,
}

pub(crate) fn build_module(module: &declarations::Module) -> Option<AggregateRegistry>
{
  let mut registry = build(&module.nominal_types)?;
  let definitions = module.nominal_types
                          .iter()
                          .filter(|declaration| declaration.kind == NominalKind::Data)
                          .map(|declaration| (declaration.name.as_str(), declaration))
                          .collect::<HashMap<_, _>>();
  for function in &module.functions
  {
    let _ = visit_type_ref(&function.return_type, &definitions, &mut HashSet::new(), &mut registry);
    for parameter in &function.parameters
    {
      let _ = visit_type_ref(&parameter.ty, &definitions, &mut HashSet::new(), &mut registry);
    }
    if let Some(body) = &function.body
    {
      visit_statements(body, &definitions, &mut registry)?;
    }
  }
  Some(registry)
}

pub(crate) fn build_functions(functions: &[super::type_check::Function]) -> Option<AggregateRegistry>
{
  let definitions = HashMap::new();
  let mut registry = AggregateRegistry::default();
  for function in functions
  {
    if let Some(return_type) = &function.return_type
    {
      let _ = visit_checked_type(return_type, &definitions, &mut HashSet::new(), &mut registry);
    }
    for local in &function.locals
    {
      let _ = visit_checked_type(&local.ty, &definitions, &mut HashSet::new(), &mut registry);
    }
    visit_statements(&function.body, &definitions, &mut registry)?;
  }
  Some(registry)
}

fn visit_statements(statements: &[Statement],
                    definitions: &HashMap<&str, &NominalType>,
                    registry: &mut AggregateRegistry)
                    -> Option<()>
{
  for statement in statements
  {
    match statement
    {
      Statement::Let { local, .. } =>
      {
        let _ = visit_checked_type(&local.ty, definitions, &mut HashSet::new(), registry);
      }
      Statement::If { then_block,
                      else_block,
                      .. } =>
      {
        visit_statements(&then_block.statements, definitions, registry)?;
        if let Some(block) = else_block
        {
          visit_statements(&block.statements, definitions, registry)?;
        }
      }
      Statement::While { body, .. } | Statement::ForEach { body, .. } =>
      {
        visit_statements(&body.statements, definitions, registry)?
      }
      Statement::For { initializer,
                       body,
                       .. } =>
      {
        if let Some(initializer) = initializer
        {
          visit_statements(std::slice::from_ref(initializer), definitions, registry)?;
        }
        visit_statements(&body.statements, definitions, registry)?;
      }
      Statement::Match { arms, .. } =>
      {
        for arm in arms
        {
          visit_statements(&arm.body.statements, definitions, registry)?;
        }
      }
      _ =>
      {}
    }
  }
  Some(())
}

fn visit_type_ref(value: &TypeRef,
                  definitions: &HashMap<&str, &NominalType>,
                  visiting: &mut HashSet<String>,
                  registry: &mut AggregateRegistry)
                  -> Option<Type>
{
  visit_checked_type(&declarations::type_ref_to_checked(value)?,
                     definitions,
                     visiting,
                     registry)
}

fn visit_checked_type(value: &HirType,
                      definitions: &HashMap<&str, &NominalType>,
                      visiting: &mut HashSet<String>,
                      registry: &mut AggregateRegistry)
                      -> Option<Type>
{
  match value
  {
    HirType::Primitive(primitive) => super::mir_lowering::primitive_to_xlil(*primitive),
    HirType::Named(name) => visit(definitions.get(name.as_str())?, definitions, visiting, registry),
    HirType::Tuple { fields } =>
    {
      if let Some((_, value_type)) = registry.tuples.iter().find(|(source, _)| source == value)
      {
        return Some(*value_type);
      }
      let field_types = fields.iter()
                              .map(|field| visit_checked_type(&field.ty, definitions, visiting, registry))
                              .collect::<Option<Vec<_>>>()?;
      let value_type = Type::aggregate(registry.layouts.len() as u32);
      registry.layouts
              .push(AggregateLayout { name: format!("tuple.{}", registry.tuples.len()),
                                      value_type,
                                      fields: field_types });
      registry.tuples.push((value.clone(), value_type));
      Some(value_type)
    }
    HirType::Array { element, .. } | HirType::Set { element } =>
    {
      let _ = visit_checked_type(element, definitions, visiting, registry);
      None
    }
    HirType::Map { key,
                   value, } =>
    {
      let _ = visit_checked_type(key, definitions, visiting, registry);
      let _ = visit_checked_type(value, definitions, visiting, registry);
      None
    }
    HirType::Unit => None,
  }
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
            TypeRef::Array { .. } | TypeRef::Map { .. } | TypeRef::Tuple { .. } => return None,
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
