/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn surface_expression_from_desugared(&mut self, expression: &DesugaredExpression) -> Option<Expression>
  {
    match expression
    {
      DesugaredExpression::Literal { literal,
                                     span, } => Some(Expression::Literal { literal: literal.clone(),
                                                                           span: *span }),
      DesugaredExpression::Local { name,
                                   span, } => Some(Expression::Local { name: name.clone(),
                                                                       span: *span }),
      DesugaredExpression::Assign { target,
                                    value,
                                    span, } =>
      {
        self.surface_expression_from_desugared(value)
            .map(|value| Expression::Assign { target: target.clone(),
                                              value: Box::new(value),
                                              span: *span })
      }
      DesugaredExpression::Binary { operator,
                                    left,
                                    right,
                                    span, } =>
      {
        Some(Expression::Binary { operator: *operator,
                                  left: Box::new(self.surface_expression_from_desugared(left)?),
                                  right: Box::new(self.surface_expression_from_desugared(right)?),
                                  span: *span })
      }
      DesugaredExpression::Unary { operator,
                                   operand,
                                   span, } =>
      {
        Some(Expression::Unary { operator: *operator,
                                 operand: Box::new(self.surface_expression_from_desugared(operand)?),
                                 span: *span })
      }
      DesugaredExpression::ResultMatch { span, .. } =>
      {
        self.report(DiagnosticCode::UnsupportedExpression,
                    "desugared Result match lowering awaits MIR Result control-flow support",
                    *span);
        None
      }
      DesugaredExpression::Call { function,
                                  arguments,
                                  parameter_types,
                                  return_type,
                                  span, } =>
      {
        Some(Expression::Call { function: function.clone(),
                                arguments: arguments.iter()
                                                    .map(|value| self.surface_expression_from_desugared(value))
                                                    .collect::<Option<Vec<_>>>()?,
                                parameter_types: parameter_types.clone(),
                                return_type: return_type.clone(),
                                span: *span })
      }
      DesugaredExpression::If { condition,
                                then_block,
                                else_block,
                                result_type,
                                span, } =>
      {
        Some(Expression::If { condition: Box::new(self.surface_expression_from_desugared(condition)?),
                              then_block: Box::new(self.surface_block_from_desugared(then_block)?),
                              else_block: Box::new(self.surface_block_from_desugared(else_block)?),
                              result_type: result_type.clone(),
                              span: *span })
      }
      DesugaredExpression::Match { selector,
                                   selector_type,
                                   arms,
                                   result_type,
                                   span, } =>
      {
        Some(Expression::Match { selector: Box::new(self.surface_expression_from_desugared(selector)?),
                                 selector_type: selector_type.clone(),
                                 arms: arms.iter()
                                           .map(|arm| {
                                             Some(MatchArm { pattern: arm.pattern.clone(),
                                                             body: self.surface_block_from_desugared(&arm.body)?,
                                                             span: arm.span })
                                           })
                                           .collect::<Option<Vec<_>>>()?,
                                 result_type: result_type.clone(),
                                 span: *span })
      }
    }
  }

  pub(super) fn surface_block_from_desugared(&mut self, block: &DesugaredBlock) -> Option<Block>
  {
    let statements = block.statements
                          .iter()
                          .map(|statement| self.surface_statement_from_desugared(statement))
                          .collect::<Option<Vec<_>>>()?;
    let tail = match &block.tail
    {
      Some(expression) => Some(Box::new(self.surface_expression_from_desugared(expression)?)),
      None => None,
    };
    Some(Block { statements,
                 tail,
                 span: block.span })
  }
}
