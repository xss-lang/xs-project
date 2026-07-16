/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::type_check::{PrimitiveType, Type};

pub(super) fn type_name(ty: &Type) -> String
{
  match ty
  {
    Type::Unit => "()".to_string(),
    Type::Primitive(primitive) => primitive_type_name(*primitive).to_string(),
    Type::Named(name) => name.clone(),
    Type::Array { element,
                  length: None, } => format!("[{}]", type_name(element)),
    Type::Array { element,
                  length: Some(length), } => format!("[{}; {length}]", type_name(element)),
    Type::Set { element } => format!("set [{}]", type_name(element)),
    Type::Map { key,
                value, } => format!("[{}: {}]", type_name(key), type_name(value)),
    Type::Tuple { fields } => format!("({})",
                                      fields.iter()
                                            .map(|field| match &field.name
                                            {
                                              Some(name) => format!("{name}: {}", type_name(&field.ty)),
                                              None => type_name(&field.ty),
                                            })
                                            .collect::<Vec<_>>()
                                            .join(", ")),
  }
}

const fn primitive_type_name(primitive: PrimitiveType) -> &'static str
{
  match primitive
  {
    PrimitiveType::Bool => "Bool",
    PrimitiveType::Byte => "Byte",
    PrimitiveType::SByte => "SByte",
    PrimitiveType::Char => "Char",
    PrimitiveType::Short => "Short",
    PrimitiveType::Long => "Long",
    PrimitiveType::Int => "Int",
    PrimitiveType::Integer => "Integer",
    PrimitiveType::UShort => "UShort",
    PrimitiveType::ULong => "ULong",
    PrimitiveType::UInt => "UInt",
    PrimitiveType::UInteger => "UInteger",
    PrimitiveType::SFloat => "SFloat",
    PrimitiveType::Float => "Float",
    PrimitiveType::Str => "Str",
  }
}
