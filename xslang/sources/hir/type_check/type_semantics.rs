/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{Literal, PrimitiveType, Type};

pub const OPTIONAL_STR_TYPE_NAME: &str = "Optional<Str>";

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ValueOwnership
{
  Value,
  BorrowedStatic,
  BoxedOwned,
}

impl Type
{
  #[must_use]
  pub fn is_boxed_optional_str(&self) -> bool
  {
    matches!(self, Self::Named(name) if name == OPTIONAL_STR_TYPE_NAME ||
      name == "std::optional::Optional<Str>")
  }

  #[must_use]
  pub fn ownership(&self) -> ValueOwnership
  {
    match self
    {
      Self::Primitive(PrimitiveType::Str) => ValueOwnership::BorrowedStatic,
      value if value.is_boxed_optional_str() => ValueOwnership::BoxedOwned,
      _ => ValueOwnership::Value,
    }
  }
}

pub(super) fn literal_default_type(literal: &Literal) -> Option<Type>
{
  let primitive = match literal
  {
    Literal::Bool(_) => PrimitiveType::Bool,
    Literal::Integer(_) => PrimitiveType::Int,
    Literal::Float(_) => PrimitiveType::Float,
    Literal::Char(_) => PrimitiveType::Char,
    Literal::String(_) => PrimitiveType::Str,
    Literal::None => return None,
    Literal::EnumVariant { enum_type, .. } => return Some(Type::Named(enum_type.clone())),
  };
  Some(Type::Primitive(primitive))
}

#[must_use]
pub fn literal_matches_type(literal: &Literal, ty: &Type) -> bool
{
  if ty.is_boxed_optional_str()
  {
    return matches!(literal, Literal::None);
  }
  if let Literal::EnumVariant { enum_type, .. } = literal
  {
    return ty == &Type::Named(enum_type.clone());
  }
  let Type::Primitive(primitive) = ty
  else
  {
    return true;
  };
  match literal
  {
    Literal::None => true,
    Literal::Bool(_) => *primitive == PrimitiveType::Bool,
    Literal::Integer(_) => matches!(primitive,
                                    PrimitiveType::Byte |
                                    PrimitiveType::SByte |
                                    PrimitiveType::Short |
                                    PrimitiveType::Long |
                                    PrimitiveType::Int |
                                    PrimitiveType::Integer |
                                    PrimitiveType::UShort |
                                    PrimitiveType::ULong |
                                    PrimitiveType::UInt |
                                    PrimitiveType::UInteger),
    Literal::Float(_) => matches!(primitive, PrimitiveType::SFloat | PrimitiveType::Float),
    Literal::Char(_) => *primitive == PrimitiveType::Char,
    Literal::String(_) => *primitive == PrimitiveType::Str,
    Literal::EnumVariant { .. } => false,
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::hir::type_check::{Literal, literal_matches_type};

  #[test]
  fn distinguishes_borrowed_str_from_boxed_optional_str()
  {
    assert_eq!(Type::Primitive(PrimitiveType::Str).ownership(),
               ValueOwnership::BorrowedStatic);
    assert_eq!(Type::Named(OPTIONAL_STR_TYPE_NAME.to_string()).ownership(),
               ValueOwnership::BoxedOwned);
    assert_eq!(Type::Primitive(PrimitiveType::Int).ownership(), ValueOwnership::Value);
  }

  #[test]
  fn canonical_optional_str_requires_boxing_before_xhir()
  {
    let boxed = Type::Named(OPTIONAL_STR_TYPE_NAME.to_string());
    assert!(literal_matches_type(&Literal::None, &boxed));
    assert!(!literal_matches_type(&Literal::String("Leitwolf".to_string()), &boxed));
  }
}
