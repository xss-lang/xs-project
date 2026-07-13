/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn lower_literal_into(&mut self,
                                   target: mir::LocalId,
                                   literal: &Literal,
                                   span: Span,
                                   lowered: &mut mir::Function)
  {
    let value_type = lowered.locals
                            .iter()
                            .find(|local| local.id == target)
                            .and_then(|local| local.value_type);
    match (literal, value_type)
    {
      (Literal::Integer(value), Some(XlilType { kind: TypeKind::I64 })) =>
      {
        let Ok(value) = value.replace('\'', "").parse::<i64>()
        else
        {
          self.report(DiagnosticCode::InvalidIntegerLiteral,
                      "HIR integer literal does not fit MIR const.i64",
                      span);
          return;
        };
        self.current_block_mut(lowered)
            .statements
            .push(mir::Statement::ConstI64 { local: target,
                                             value,
                                             span });
      }
      (Literal::Integer(value), Some(XlilType { kind: TypeKind::I32 })) =>
      {
        let Ok(value) = value.replace('\'', "").parse::<i32>()
        else
        {
          self.report(DiagnosticCode::InvalidIntegerLiteral,
                      "HIR integer literal does not fit MIR const.i32",
                      span);
          return;
        };
        self.current_block_mut(lowered)
            .statements
            .push(mir::Statement::ConstI32 { local: target,
                                             value,
                                             span });
      }
      (Literal::Bool(value), Some(XlilType { kind: TypeKind::Bool })) =>
      {
        self.current_block_mut(lowered)
            .statements
            .push(mir::Statement::ConstBool { local: target,
                                              value: *value,
                                              span });
      }
      (Literal::Integer(_), Some(_)) => self.report(DiagnosticCode::UnsupportedType,
                                                    "only Long and Int literals can lower to MIR constants today",
                                                    span),
      (Literal::None, _) => self.report(DiagnosticCode::UnsupportedExpression,
                                        "Optional None lowering is not implemented in MIR yet",
                                        span),
      _ => self.report(DiagnosticCode::UnsupportedExpression,
                       "HIR literal kind cannot lower to MIR yet",
                       span),
    }
  }

  pub(super) fn declare_local(&mut self,
                              name: String,
                              ty: &Type,
                              mutable: bool,
                              span: Span,
                              lowered: &mut mir::Function)
                              -> mir::LocalId
  {
    if let Some(id) = self.locals.get(&name).copied()
    {
      return id;
    }
    let id = mir::LocalId(self.next_local);
    self.next_local += 1;
    let value_type = self.lower_value_type(ty, span);
    lowered.locals.push(mir::Local { id,
                                     name: name.clone(),
                                     value_type,
                                     mutable,
                                     span });
    self.locals.insert(name, id);
    self.storage_locals.insert(id);
    id
  }

  pub(super) fn declare_temp(&mut self,
                             value_type: XlilType,
                             span: Span,
                             lowered: &mut mir::Function)
                             -> Option<mir::LocalId>
  {
    if value_type == XlilType::VOID
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "void return value cannot allocate a MIR local",
                  span);
      return None;
    }
    let id = mir::LocalId(self.next_local);
    self.next_local += 1;
    lowered.locals.push(mir::Local { id,
                                     name: format!("$tmp{}", id.0),
                                     value_type: Some(value_type),
                                     mutable: false,
                                     span });
    Some(id)
  }

  pub(super) fn lower_return_type(&mut self, ty: Option<&Type>, span: Span) -> XlilType
  {
    ty.and_then(|ty| self.lower_value_type(ty, span))
      .unwrap_or(XlilType::VOID)
  }

  pub(super) fn lower_value_type(&mut self, ty: &Type, span: Span) -> Option<XlilType>
  {
    let Type::Primitive(primitive) = ty
    else
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "user-defined HIR type has no MIR value model yet",
                  span);
      return None;
    };
    primitive_to_xlil(*primitive).or_else(|| {
                                   self.report(DiagnosticCode::UnsupportedType,
                                               "HIR primitive has no XLIL-backed MIR value type yet",
                                               span);
                                   None
                                 })
  }
}
