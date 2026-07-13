/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::PrimitiveType;

pub(super) fn primitive_type(name: &str) -> Option<PrimitiveType>
{
  Some(match name
  {
    "Bool" => PrimitiveType::Bool,
    "Byte" => PrimitiveType::Byte,
    "SByte" => PrimitiveType::SByte,
    "Char" => PrimitiveType::Char,
    "Short" => PrimitiveType::Short,
    "Long" => PrimitiveType::Long,
    "Int" => PrimitiveType::Int,
    "Integer" => PrimitiveType::Integer,
    "UShort" => PrimitiveType::UShort,
    "ULong" => PrimitiveType::ULong,
    "UInt" => PrimitiveType::UInt,
    "UInteger" => PrimitiveType::UInteger,
    "SFloat" => PrimitiveType::SFloat,
    "Float" => PrimitiveType::Float,
    "Str" => PrimitiveType::Str,
    _ => return None,
  })
}

pub(super) fn is_named_type(name: &str) -> bool
{
  let name = name.trim();
  if !name.starts_with(|value: char| value == '_' || value.is_alphabetic()) ||
     name.ends_with([',', '<', ':', '.']) ||
     ["<>", "<,", ",>", ",,"].iter().any(|pattern| name.contains(pattern))
  {
    return false;
  }
  let mut generic_depth = 0_u32;
  let mut saw_identifier = false;
  for character in name.chars()
  {
    match character
    {
      '<' => generic_depth += 1,
      '>' if generic_depth > 0 => generic_depth -= 1,
      '>' | ',' if generic_depth == 0 => return false,
      '_' | '.' | ':' | ',' | ' ' | '(' | ')' =>
      {}
      value if value.is_alphanumeric() => saw_identifier = true,
      _ => return false,
    }
  }
  saw_identifier && generic_depth == 0
}

pub(super) fn split_type_list(text: &str) -> Vec<&str>
{
  let mut result = Vec::new();
  let mut depth = 0_u32;
  let mut start = 0;
  for (index, character) in text.char_indices()
  {
    match character
    {
      '<' => depth += 1,
      '>' => depth = depth.saturating_sub(1),
      ',' if depth == 0 =>
      {
        result.push(&text[start..index]);
        start = index + character.len_utf8();
      }
      _ =>
      {}
    }
  }
  result.push(&text[start..]);
  result
}
