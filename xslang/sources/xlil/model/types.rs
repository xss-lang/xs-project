/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
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
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Type
{
  pub kind: TypeKind,
}

impl Type
{
  pub const VOID: Self = Self { kind: TypeKind::Void };
  pub const BOOL: Self = Self { kind: TypeKind::Bool };
  pub const U16: Self = Self { kind: TypeKind::U16 };
  pub const I32: Self = Self { kind: TypeKind::I32 };
  pub const I64: Self = Self { kind: TypeKind::I64 };
  pub const F32: Self = Self { kind: TypeKind::F32 };
  pub const F64: Self = Self { kind: TypeKind::F64 };
  pub const STR: Self = Self { kind: TypeKind::Str };
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
