/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::declarations::{Module as HirModule, TypeRef};
use crate::hir::type_check::PrimitiveType;
use crate::mir;
use crate::xlil::{self, Function, Module, Type};

fn lower_type(value: &TypeRef) -> Option<Type>
{
  match value
  {
    TypeRef::Unit => Some(Type::VOID),
    TypeRef::Primitive(value) => crate::hir::mir_lowering::primitive_to_xlil(*value),
    TypeRef::Named(_) => None,
  }
}

fn signature(function: &crate::hir::declarations::Function) -> Option<(Type, Vec<Type>)>
{
  let return_type = lower_type(&function.return_type)?;
  let parameters = function.parameters
                           .iter()
                           .map(|parameter| lower_type(&parameter.ty))
                           .collect::<Option<Vec<_>>>()?;
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

fn supports_source_native_subset(module: &HirModule) -> bool
{
  module.functions.iter().all(|function| {
                           let parameters_supported = function.parameters.iter().all(|parameter| {
                                                                                  matches!(parameter.ty,
                                                               TypeRef::Primitive(PrimitiveType::Long |
                                                                                  PrimitiveType::Int |
                                                                                  PrimitiveType::Bool |
                                                                                  PrimitiveType::Byte |
                                                                                  PrimitiveType::SByte |
                                                                                  PrimitiveType::Char |
                                                                                  PrimitiveType::Short |
                                                                                  PrimitiveType::Integer |
                                                                                  PrimitiveType::UShort |
                                                                                  PrimitiveType::ULong |
                                                                                  PrimitiveType::UInt |
                                                                                  PrimitiveType::UInteger |
                                                                                  PrimitiveType::SFloat |
                                                                                  PrimitiveType::Float |
                                                                                  PrimitiveType::Str))
                                                                                });
                           let return_supported = matches!(function.return_type,
                                                           TypeRef::Unit |
                                                           TypeRef::Primitive(PrimitiveType::Long |
                                                                              PrimitiveType::Int |
                                                                              PrimitiveType::Bool |
                                                                              PrimitiveType::Byte |
                                                                              PrimitiveType::SByte |
                                                                              PrimitiveType::Char |
                                                                              PrimitiveType::Short |
                                                                              PrimitiveType::Integer |
                                                                              PrimitiveType::UShort |
                                                                              PrimitiveType::ULong |
                                                                              PrimitiveType::UInt |
                                                                              PrimitiveType::UInteger |
                                                                              PrimitiveType::SFloat |
                                                                              PrimitiveType::Float |
                                                                              PrimitiveType::Str));
                           !function.body_present || (parameters_supported && return_supported)
                         })
}

pub(super) fn lower_module(declarations: &HirModule, mir_functions: &[mir::Function]) -> Option<Module>
{
  if !supports_source_native_subset(declarations)
  {
    return None;
  }
  let mut module = Module::new(declarations.name.clone().unwrap_or_else(|| "root".to_string()));
  let mut used_mir = vec![false; mir_functions.len()];
  for declaration in &declarations.functions
  {
    let (return_type, parameters) = signature(declaration)?;
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
