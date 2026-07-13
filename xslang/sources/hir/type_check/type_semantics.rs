/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{PrimitiveType, Type};

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
