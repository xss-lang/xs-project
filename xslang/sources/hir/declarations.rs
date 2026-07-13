/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::compiler_core::SourceSpan;

use super::type_check::PrimitiveType;

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum TypeRef
{
  Unit,
  Primitive(PrimitiveType),
  Named(String),
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Parameter
{
  pub name: String,
  pub ty: TypeRef,
  pub span: SourceSpan,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Function
{
  pub name: String,
  pub parameters: Vec<Parameter>,
  pub return_type: TypeRef,
  pub flags: u32,
  pub span: SourceSpan,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Module
{
  pub name: Option<String>,
  pub functions: Vec<Function>,
}
