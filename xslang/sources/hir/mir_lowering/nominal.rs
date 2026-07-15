/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::hir::type_check::{FieldPath, Local};

impl HirToMirLowerer
{
  pub(super) fn lower_nominal_binding(&mut self,
                                      local: &Local,
                                      initializer: Option<&Expression>,
                                      lowered: &mut mir::Function)
  {
    let Type::Named(type_name) = &local.ty
    else
    {
      return;
    };
    let Some(definition) = self.nominal_types.get(type_name).cloned()
    else
    {
      self.report(DiagnosticCode::UnsupportedType,
                  format!("nominal type '{type_name}' has no HIR declaration"),
                  local.span);
      return;
    };
    let Some(Expression::Object { nominal_type,
                                  fields,
                                  .. }) = initializer
    else
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "nominal local requires an object initializer before aggregate default construction is available",
                  local.span);
      return;
    };
    if nominal_type != type_name
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "object initializer type does not match nominal local type",
                  local.span);
      return;
    }
    for field in &definition.fields
    {
      let Some(value_type) = crate::hir::declarations::type_ref_to_checked(&field.ty)
      else
      {
        continue;
      };
      if !matches!(value_type, Type::Primitive(_))
      {
        self.report(DiagnosticCode::UnsupportedType,
                    format!("field '{}.{}' is not scalar-lowerable yet", type_name, field.name),
                    local.span);
        continue;
      }
      let Some(initializer) = fields.iter().find(|candidate| candidate.name == field.name)
      else
      {
        continue;
      };
      let key = field_key(&local.name, std::slice::from_ref(&field.name));
      let field_local = self.declare_local(key.clone(), &value_type, field.mutable, initializer.span, lowered);
      self.field_locals.insert(key, field_local);
      self.lower_assignment(field_local, &initializer.value, lowered);
    }
  }

  pub(super) fn lower_field_load(&mut self,
                                 path: &FieldPath,
                                 expected_type: XlilType,
                                 lowered: &mut mir::Function)
                                 -> Option<mir::LocalId>
  {
    let key = field_key(&path.root, &path.fields);
    let Some(local) = self.field_locals.get(&key).copied()
    else
    {
      self.report(DiagnosticCode::UnknownLocal,
                  format!("field place '{key}' has not been initialized"),
                  path.span);
      return None;
    };
    if self.local_value_type(local, lowered) != Some(expected_type)
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "field place type does not match the expected MIR type",
                  path.span);
      return None;
    }
    let result = self.declare_temp(expected_type, path.span, lowered)?;
    self.current_block_mut(lowered)
        .statements
        .push(mir::Statement::LoadLocal { result,
                                          local,
                                          span: path.span });
    Some(result)
  }

  pub(super) fn lower_field_assignment(&mut self, target: &FieldPath, value: &Expression, lowered: &mut mir::Function)
  {
    let key = field_key(&target.root, &target.fields);
    let Some(local) = self.field_locals.get(&key).copied()
    else
    {
      self.report(DiagnosticCode::UnknownLocal,
                  format!("field place '{key}' has not been initialized"),
                  target.span);
      return;
    };
    self.lower_assignment(local, value, lowered);
  }
}

fn field_key(root: &str, fields: &[String]) -> String
{
  std::iter::once(root).chain(fields.iter().map(String::as_str))
                       .collect::<Vec<_>>()
                       .join(".")
}
