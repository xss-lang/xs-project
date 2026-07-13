/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn lower_call_into(&mut self, target: mir::LocalId, expression: &Expression, lowered: &mut mir::Function)
  {
    let Expression::Call { function,
                           arguments,
                           parameter_types,
                           return_type,
                           span, } = expression
    else
    {
      return;
    };
    if arguments.len() != parameter_types.len()
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "typed HIR call argument count does not match parameter types",
                  *span);
      return;
    }
    let Some(return_type) = self.lower_value_type(return_type, *span)
    else
    {
      return;
    };
    if self.local_value_type(target, lowered) != Some(return_type)
    {
      self.report(DiagnosticCode::UnsupportedType,
                  "typed HIR call result type does not match its target local",
                  *span);
      return;
    }
    let mut lowered_arguments = Vec::with_capacity(arguments.len());
    for (argument, parameter_type) in arguments.iter().zip(parameter_types)
    {
      let Some(parameter_type) = self.lower_value_type(parameter_type, *span)
      else
      {
        return;
      };
      let Some(argument) = self.lower_expression_to_local(argument, parameter_type, lowered)
      else
      {
        return;
      };
      lowered_arguments.push(argument);
    }
    lowered.blocks[0].statements
                     .push(mir::Statement::Call { result: Some(target),
                                                  function: function.clone(),
                                                  arguments: lowered_arguments,
                                                  return_type,
                                                  span: *span });
  }
}
