/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::compiler_core::SourceSpan;

use super::type_check::{self, PrimitiveType};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum TypeRef
{
  Unit,
  Primitive(PrimitiveType),
  Named(String),
}

#[must_use]
pub fn type_ref_to_checked(value: &TypeRef) -> Option<type_check::Type>
{
  Some(match value
  {
    TypeRef::Unit => type_check::Type::Unit,
    TypeRef::Primitive(value) => type_check::Type::Primitive(*value),
    TypeRef::Named(value) => type_check::Type::Named(value.clone()),
  })
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Parameter
{
  pub name: String,
  pub ty: TypeRef,
  pub span: SourceSpan,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum NominalKind
{
  Class,
  Data,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Field
{
  pub name: String,
  pub ty: TypeRef,
  pub mutable: bool,
  pub span: SourceSpan,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct NominalType
{
  pub name: String,
  pub kind: NominalKind,
  pub fields: Vec<Field>,
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
  pub body_present: bool,
  pub body: Option<Vec<type_check::Statement>>,
}

impl Function
{
  #[must_use]
  pub fn as_type_checked_input(&self) -> Option<type_check::Function>
  {
    let body = self.body.clone()?;
    let return_type = if self.return_type == TypeRef::Unit
    {
      None
    }
    else
    {
      Some(type_ref_to_checked(&self.return_type)?)
    };
    let locals = self.parameters
                     .iter()
                     .map(|parameter| {
                       Some(type_check::Local { name: parameter.name.clone(),
                                                               ty: type_ref_to_checked(&parameter.ty)?,
                                                               mutable: false,
                                                               span: crate::hir::async_check::Span::new(
                                                                 parameter.span.file_id,
                                                                 u32::try_from(parameter.span.start_offset).ok()?,
                                                                 u32::try_from(parameter.span.end_offset).ok()?), })
                     })
                     .collect::<Option<Vec<_>>>()?;
    Some(type_check::Function { name: self.name.clone(),
                                return_type,
                                locals,
                                body })
  }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Module
{
  pub name: Option<String>,
  pub nominal_types: Vec<NominalType>,
  pub functions: Vec<Function>,
}
