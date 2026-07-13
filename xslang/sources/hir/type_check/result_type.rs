/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{PrimitiveType, Type};

pub(crate) fn result_type_parts(ty: &Type) -> Option<(Type, Type)>
{
  let Type::Named(name) = ty
  else
  {
    return None;
  };
  let generic = name.strip_prefix("Result<")
                    .or_else(|| name.strip_prefix("Result.Result<"))?;
  let inner = generic.strip_suffix('>')?;
  if let Some((success, error)) = split_top_level_pair(inner)
  {
    return Some((parse_named_or_primitive(success.trim()), parse_named_or_primitive(error.trim())));
  }
  Some((parse_named_or_primitive(inner.trim()), Type::Named("Result.Error".to_string())))
}

fn split_top_level_pair(text: &str) -> Option<(&str, &str)>
{
  let mut depth = 0usize;
  for (index, ch) in text.char_indices()
  {
    match ch
    {
      '<' => depth += 1,
      '>' => depth = depth.saturating_sub(1),
      ',' if depth == 0 => return Some((&text[..index], &text[index + 1..])),
      _ =>
      {}
    }
  }
  None
}

fn parse_named_or_primitive(name: &str) -> Type
{
  let primitive = match name
  {
    "Bool" => Some(PrimitiveType::Bool),
    "Byte" => Some(PrimitiveType::Byte),
    "SByte" => Some(PrimitiveType::SByte),
    "Char" => Some(PrimitiveType::Char),
    "Short" => Some(PrimitiveType::Short),
    "Long" => Some(PrimitiveType::Long),
    "Int" => Some(PrimitiveType::Int),
    "Integer" => Some(PrimitiveType::Integer),
    "UShort" => Some(PrimitiveType::UShort),
    "ULong" => Some(PrimitiveType::ULong),
    "UInt" => Some(PrimitiveType::UInt),
    "UInteger" => Some(PrimitiveType::UInteger),
    "SFloat" => Some(PrimitiveType::SFloat),
    "Float" => Some(PrimitiveType::Float),
    "Str" => Some(PrimitiveType::Str),
    _ => None,
  };
  primitive.map_or_else(|| Type::Named(name.to_string()), Type::Primitive)
}
