/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum TypeKind
{
  Void,
  Bool,
  U8,
  I8,
  U16,
  I16,
  U32,
  I32,
  U64,
  I64,
  U128,
  I128,
  F16,
  F32,
  F64,
  F128,
  Str,
  Aggregate,
  Array,
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct Type
{
  pub kind: TypeKind,
  pub registry_id: u32,
}

impl Type
{
  const fn primitive(kind: TypeKind) -> Self
  {
    Self { kind,
           registry_id: 0 }
  }

  pub const VOID: Self = Self::primitive(TypeKind::Void);
  pub const BOOL: Self = Self::primitive(TypeKind::Bool);
  pub const U8: Self = Self::primitive(TypeKind::U8);
  pub const I8: Self = Self::primitive(TypeKind::I8);
  pub const U16: Self = Self::primitive(TypeKind::U16);
  pub const I16: Self = Self::primitive(TypeKind::I16);
  pub const U32: Self = Self::primitive(TypeKind::U32);
  pub const I32: Self = Self::primitive(TypeKind::I32);
  pub const U64: Self = Self::primitive(TypeKind::U64);
  pub const I64: Self = Self::primitive(TypeKind::I64);
  pub const U128: Self = Self::primitive(TypeKind::U128);
  pub const I128: Self = Self::primitive(TypeKind::I128);
  pub const F32: Self = Self::primitive(TypeKind::F32);
  pub const F64: Self = Self::primitive(TypeKind::F64);
  pub const STR: Self = Self::primitive(TypeKind::Str);

  #[must_use]
  pub const fn aggregate(registry_id: u32) -> Self
  {
    Self { kind: TypeKind::Aggregate,
           registry_id }
  }

  #[must_use]
  pub const fn array(registry_id: u32) -> Self
  {
    Self { kind: TypeKind::Array,
           registry_id }
  }

  #[must_use]
  pub const fn integer_width(self) -> Option<u32>
  {
    match self.kind
    {
      TypeKind::U8 | TypeKind::I8 => Some(8),
      TypeKind::U16 | TypeKind::I16 => Some(16),
      TypeKind::U32 | TypeKind::I32 => Some(32),
      TypeKind::U64 | TypeKind::I64 => Some(64),
      TypeKind::U128 | TypeKind::I128 => Some(128),
      _ => None,
    }
  }

  #[must_use]
  pub const fn is_integer(self) -> bool
  {
    self.integer_width().is_some()
  }

  #[must_use]
  pub const fn is_signed_integer(self) -> bool
  {
    matches!(self.kind,
             TypeKind::I8 | TypeKind::I16 | TypeKind::I32 | TypeKind::I64 | TypeKind::I128)
  }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Utf16Encoding
{
  LittleEndian,
  BigEndian,
}

impl Utf16Encoding
{
  #[must_use]
  pub const fn native() -> Self
  {
    if cfg!(target_endian = "little")
    {
      Self::LittleEndian
    }
    else
    {
      Self::BigEndian
    }
  }

  #[must_use]
  pub const fn text_name(self) -> &'static str
  {
    match self
    {
      Self::LittleEndian => "utf16le",
      Self::BigEndian => "utf16be",
    }
  }
}

impl Default for Utf16Encoding
{
  fn default() -> Self
  {
    Self::native()
  }
}
