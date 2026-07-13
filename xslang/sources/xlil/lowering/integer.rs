/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl MirToXlilLowerer
{
  #[allow(clippy::too_many_arguments)]
  pub(super) fn lower_const_u16(&mut self,
                                local: mir::LocalId,
                                value: u16,
                                span: Span,
                                block: BlockId,
                                local_types: &HashMap<mir::LocalId, Option<Type>>,
                                values: &mut HashMap<mir::LocalId, ValueId>,
                                function: &mut Function)
  {
    match local_types.get(&local).copied().flatten()
    {
      Some(Type::U16) =>
      {
        if let Some(value_id) = function.add_const_u16(block, value)
        {
          values.insert(local, value_id);
        }
      }
      Some(_) => self.report(DiagnosticCode::UnsupportedLocalType,
                             "MIR const.u16 target local must have XLIL u16 type",
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "MIR const.u16 target local has no XLIL value type",
                          span),
    }
  }

  #[allow(clippy::too_many_arguments)]
  pub(super) fn lower_const_integer(&mut self,
                                    local: mir::LocalId,
                                    value: mir::IntegerConstant,
                                    span: Span,
                                    block: BlockId,
                                    local_types: &HashMap<mir::LocalId, Option<Type>>,
                                    values: &mut HashMap<mir::LocalId, ValueId>,
                                    function: &mut Function)
  {
    let expected = value.value_type();
    match local_types.get(&local).copied().flatten()
    {
      Some(actual) if actual == expected =>
      {
        let constant = IntegerConstant::new(expected, value.bits()).expect("verified MIR integer fits its type");
        if let Some(value_id) = function.add_const_integer(block, constant)
        {
          values.insert(local, value_id);
        }
      }
      Some(_) => self.report(DiagnosticCode::UnsupportedLocalType,
                             "MIR integer constant target local has the wrong XLIL type",
                             span),
      None => self.report(DiagnosticCode::MissingLocalType,
                          "MIR integer constant target local has no XLIL value type",
                          span),
    }
  }
}
