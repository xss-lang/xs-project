/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::declarations::{Module as HirModule, NominalKind, TypeRef};
use crate::mir;
use crate::xlil::{self, Function, Module, Type};

fn lower_type(value: &TypeRef, aggregates: &crate::hir::aggregate_registry::AggregateRegistry) -> Option<Type>
{
  match value
  {
    TypeRef::Unit => Some(Type::VOID),
    TypeRef::Primitive(value) => crate::hir::mir_lowering::primitive_to_xlil(*value),
    TypeRef::Named(name) => aggregates.types.get(name).copied(),
  }
}

fn flatten_parameter(module: &HirModule,
                     value: &TypeRef,
                     visiting: &mut Vec<String>,
                     parameters: &mut Vec<Type>)
                     -> Option<()>
{
  match value
  {
    TypeRef::Primitive(value) => parameters.push(crate::hir::mir_lowering::primitive_to_xlil(*value)?),
    TypeRef::Named(name) =>
    {
      if visiting.contains(name)
      {
        return None;
      }
      let definition = module.nominal_types
                             .iter()
                             .find(|definition| definition.name == *name && definition.kind == NominalKind::Data)?;
      visiting.push(name.clone());
      for field in &definition.fields
      {
        flatten_parameter(module, &field.ty, visiting, parameters)?;
      }
      visiting.pop();
    }
    TypeRef::Unit => return None,
  }
  Some(())
}

fn signature(module: &HirModule,
             aggregates: &crate::hir::aggregate_registry::AggregateRegistry,
             function: &crate::hir::declarations::Function)
             -> Option<(Type, Vec<Type>)>
{
  let return_type = lower_type(&function.return_type, aggregates)?;
  let mut parameters = Vec::new();
  for parameter in &function.parameters
  {
    flatten_parameter(module, &parameter.ty, &mut Vec::new(), &mut parameters)?;
  }
  Some((return_type, parameters))
}

fn matches_signature(function: &mir::Function, name: &str, return_type: Type, parameters: &[Type]) -> bool
{
  function.name == name &&
  function.return_type == return_type &&
  function.parameters
          .iter()
          .map(|parameter| parameter.value_type)
          .eq(parameters.iter().copied())
}

fn supports_source_native_subset(module: &HirModule,
                                 aggregates: &crate::hir::aggregate_registry::AggregateRegistry)
                                 -> bool
{
  module.functions
        .iter()
        .all(|function| signature(module, aggregates, function).is_some())
}

pub(super) fn lower_module(declarations: &HirModule, mir_functions: &[mir::Function]) -> Option<Module>
{
  let aggregates = crate::hir::aggregate_registry::build(&declarations.nominal_types)?;
  if !supports_source_native_subset(declarations, &aggregates)
  {
    return None;
  }
  let mut module = Module::new(declarations.name.clone().unwrap_or_else(|| "root".to_string()));
  for layout in &aggregates.layouts
  {
    if module.add_aggregate_type(layout.name.clone(), layout.fields.clone()) != Some(layout.value_type)
    {
      return None;
    }
  }
  let mut used_mir = vec![false; mir_functions.len()];
  for declaration in &declarations.functions
  {
    let (return_type, parameters) = signature(declarations, &aggregates, declaration)?;
    if declaration.body_present
    {
      let index =
        mir_functions.iter()
                     .enumerate()
                     .position(|(index, function)| {
                       !used_mir[index] && matches_signature(function, &declaration.name, return_type, &parameters)
                     })?;
      used_mir[index] = true;
      module.add_function(xlil::lowering::MirToXlilLowerer::new().lower_function(&mir_functions[index])
                                                                 .ok()?);
    }
    else
    {
      module.add_function(Function::declaration(declaration.name.clone(), return_type, parameters));
    }
  }
  if used_mir.iter().filter(|used| **used).count() != mir_functions.len() ||
     !xlil::verify::verify_module(&module).is_empty()
  {
    return None;
  }
  Some(module)
}
