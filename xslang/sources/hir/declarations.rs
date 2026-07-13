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
  pub body: Option<Vec<type_check::Statement>>,
}

impl Function
{
  #[must_use]
  pub fn as_type_checked_input(&self) -> Option<type_check::Function>
  {
    let body = self.body.clone()?;
    let return_type = match &self.return_type
    {
      TypeRef::Unit => None,
      TypeRef::Primitive(value) => Some(type_check::Type::Primitive(*value)),
      TypeRef::Named(value) => Some(type_check::Type::Named(value.clone())),
    };
    let locals = self.parameters
                     .iter()
                     .map(|parameter| {
                       Some(type_check::Local { name: parameter.name.clone(),
                                                               ty: match &parameter.ty
                                                               {
                                                                 TypeRef::Primitive(value) =>
                                                                   type_check::Type::Primitive(*value),
                                                                 TypeRef::Named(value) =>
                                                                   type_check::Type::Named(value.clone()),
                                                                 TypeRef::Unit => return None,
                                                               },
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
  pub functions: Vec<Function>,
}
