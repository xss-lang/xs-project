/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl HirToMirLowerer
{
  pub(super) fn lower_match_statement(&mut self, statement: &Statement, lowered: &mut mir::Function)
  {
    let Statement::Match { selector,
                           selector_type,
                           arms,
                           span, } = statement
    else
    {
      return;
    };
    if !matches!(arms.last().map(|arm| &arm.pattern), Some(MatchPattern::Else))
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "MIR match lowering requires a final else arm",
                  *span);
      return;
    }
    let selector_type = match selector_type
    {
      Type::Primitive(PrimitiveType::Bool) => XlilType::BOOL,
      Type::Primitive(PrimitiveType::Long) => XlilType::I32,
      _ =>
      {
        self.report(DiagnosticCode::UnsupportedType,
                    "MIR match lowering currently supports Bool and Long selectors",
                    *span);
        return;
      }
    };
    let Some(selector) = self.lower_expression_to_local(selector, selector_type, lowered)
    else
    {
      return;
    };
    let merge = self.append_block(*span, lowered);
    let outer_locals = self.locals.clone();
    let mut test = self.current_block;
    let mut has_open_arm = false;
    let mut final_arm_end = self.current_block;
    for arm in arms
    {
      self.locals.clone_from(&outer_locals);
      self.switch_to(test);
      match &arm.pattern
      {
        MatchPattern::Literal(literal) =>
        {
          let body = self.append_block(arm.body.span, lowered);
          let next = self.append_block(arm.span, lowered);
          let condition = if selector_type == XlilType::BOOL
          {
            selector
          }
          else
          {
            let Some(pattern) = self.declare_temp(XlilType::I32, arm.span, lowered)
            else
            {
              return;
            };
            self.lower_literal_into(pattern, literal, arm.span, lowered);
            let Some(condition) = self.declare_temp(XlilType::BOOL, arm.span, lowered)
            else
            {
              return;
            };
            self.current_block_mut(lowered)
                .statements
                .push(mir::Statement::EqI32 { result: condition,
                                              left: selector,
                                              right: pattern,
                                              span: arm.span });
            condition
          };
          let (then_block, else_block) = if matches!(literal, Literal::Bool(false))
          {
            (next, body)
          }
          else
          {
            (body, next)
          };
          self.set_terminator(mir::Terminator::BranchIf { condition,
                                                          then_block,
                                                          else_block },
                              arm.span,
                              lowered);
          self.switch_to(body);
          self.lower_block_statements(&arm.body, lowered);
          final_arm_end = self.current_block;
          if !self.current_is_terminated(lowered)
          {
            self.set_terminator(mir::Terminator::Goto(merge), arm.span, lowered);
            has_open_arm = true;
          }
          test = next;
        }
        MatchPattern::Else =>
        {
          self.lower_block_statements(&arm.body, lowered);
          final_arm_end = self.current_block;
          if !self.current_is_terminated(lowered)
          {
            self.set_terminator(mir::Terminator::Goto(merge), arm.span, lowered);
            has_open_arm = true;
          }
        }
      }
    }
    self.locals = outer_locals;
    if has_open_arm
    {
      self.switch_to(merge);
    }
    else
    {
      lowered.blocks.retain(|block| block.id != merge);
      self.switch_to(final_arm_end);
    }
  }

  pub(super) fn lower_while_statement(&mut self, statement: &Statement, lowered: &mut mir::Function)
  {
    let Statement::While { condition,
                           body,
                           span, } = statement
    else
    {
      return;
    };
    if self.current_is_terminated(lowered)
    {
      return;
    }
    let preheader = self.current_block;
    let header = self.append_block(*span, lowered);
    let body_id = self.append_block(body.span, lowered);
    let exit = self.append_block(*span, lowered);
    self.switch_to(preheader);
    self.set_terminator(mir::Terminator::Goto(header), *span, lowered);

    self.switch_to(header);
    let Some(condition) = self.lower_expression_to_local(condition, XlilType::BOOL, lowered)
    else
    {
      return;
    };
    self.set_terminator(mir::Terminator::BranchIf { condition,
                                                    then_block: body_id,
                                                    else_block: exit },
                        *span,
                        lowered);

    let outer_locals = self.locals.clone();
    self.loop_targets.push((header, exit));
    self.switch_to(body_id);
    self.lower_block_statements(body, lowered);
    if !self.current_is_terminated(lowered)
    {
      self.set_terminator(mir::Terminator::Goto(header), body.span, lowered);
    }
    self.loop_targets.pop();
    self.locals = outer_locals;
    self.switch_to(exit);
  }

  pub(super) fn lower_for_statement(&mut self, statement: &Statement, lowered: &mut mir::Function)
  {
    let Statement::For { initializer,
                         condition,
                         update,
                         body,
                         span, } = statement
    else
    {
      return;
    };
    if self.current_is_terminated(lowered)
    {
      return;
    }

    let outer_locals = self.locals.clone();
    if let Some(initializer) = initializer
    {
      self.lower_statement(initializer, lowered);
    }
    let loop_locals = self.locals.clone();
    let preheader = self.current_block;
    let header = self.append_block(*span, lowered);
    let body_id = self.append_block(body.span, lowered);
    let update_id = self.append_block(*span, lowered);
    let exit = self.append_block(*span, lowered);
    self.switch_to(preheader);
    self.set_terminator(mir::Terminator::Goto(header), *span, lowered);

    self.switch_to(header);
    let condition = match condition
    {
      Some(condition) => self.lower_expression_to_local(condition, XlilType::BOOL, lowered),
      None =>
      {
        let result = self.declare_temp(XlilType::BOOL, *span, lowered);
        if let Some(result) = result
        {
          self.lower_literal_into(result, &Literal::Bool(true), *span, lowered);
        }
        result
      }
    };
    let Some(condition) = condition
    else
    {
      return;
    };
    self.set_terminator(mir::Terminator::BranchIf { condition,
                                                    then_block: body_id,
                                                    else_block: exit },
                        *span,
                        lowered);

    self.loop_targets.push((update_id, exit));
    self.switch_to(body_id);
    self.lower_block_statements(body, lowered);
    if !self.current_is_terminated(lowered)
    {
      self.set_terminator(mir::Terminator::Goto(update_id), body.span, lowered);
    }
    self.loop_targets.pop();

    self.locals.clone_from(&loop_locals);
    self.switch_to(update_id);
    if let Some(update) = update
    {
      self.lower_statement(&Statement::Expr(update.clone()), lowered);
    }
    if !self.current_is_terminated(lowered)
    {
      self.set_terminator(mir::Terminator::Goto(header), *span, lowered);
    }

    self.locals = outer_locals;
    self.switch_to(exit);
  }

  pub(super) fn lower_loop_jump(&mut self, is_continue: bool, span: Span, lowered: &mut mir::Function)
  {
    let Some((header, exit)) = self.loop_targets.last().copied()
    else
    {
      self.report(DiagnosticCode::UnsupportedExpression,
                  "loop jump appears outside a loop",
                  span);
      return;
    };
    let target = if is_continue
    {
      header
    }
    else
    {
      exit
    };
    self.set_terminator(mir::Terminator::Goto(target), span, lowered);
  }

  pub(super) fn lower_if_statement(&mut self, statement: &Statement, lowered: &mut mir::Function)
  {
    let Statement::If { condition,
                        then_block,
                        else_block,
                        span, } = statement
    else
    {
      return;
    };
    if self.current_is_terminated(lowered)
    {
      return;
    }
    let Some(condition) = self.lower_expression_to_local(condition, XlilType::BOOL, lowered)
    else
    {
      return;
    };
    let branch_block = self.current_block;
    let then_id = self.append_block(then_block.span, lowered);
    let else_span = else_block.as_ref().map_or(*span, |block| block.span);
    let else_id = self.append_block(else_span, lowered);
    self.switch_to(branch_block);
    self.set_terminator(mir::Terminator::BranchIf { condition,
                                                    then_block: then_id,
                                                    else_block: else_id },
                        *span,
                        lowered);

    let outer_locals = self.locals.clone();
    self.switch_to(then_id);
    self.lower_block_statements(then_block, lowered);
    let then_end = self.current_block;
    let then_open = !self.current_is_terminated(lowered);

    self.locals.clone_from(&outer_locals);
    self.switch_to(else_id);
    if let Some(else_block) = else_block
    {
      self.lower_block_statements(else_block, lowered);
    }
    let else_end = self.current_block;
    let else_open = !self.current_is_terminated(lowered);
    self.locals = outer_locals;

    if then_open || else_open
    {
      let merge = self.append_block(*span, lowered);
      if then_open
      {
        self.switch_to(then_end);
        self.set_terminator(mir::Terminator::Goto(merge), *span, lowered);
      }
      if else_open
      {
        self.switch_to(else_end);
        self.set_terminator(mir::Terminator::Goto(merge), *span, lowered);
      }
      self.switch_to(merge);
    }
    else
    {
      self.switch_to(else_end);
    }
  }

  pub(super) fn lower_if_return(&mut self, expression: &Expression, return_span: Span, lowered: &mut mir::Function)
  {
    let Expression::If { condition,
                         then_block,
                         else_block,
                         span,
                         .. } = expression
    else
    {
      return;
    };
    let Some(condition) = self.lower_expression_to_local(condition, XlilType::BOOL, lowered)
    else
    {
      return;
    };
    let branch_block = self.current_block;
    let then_id = self.append_block(then_block.span, lowered);
    let else_id = self.append_block(else_block.span, lowered);
    self.switch_to(branch_block);
    self.set_terminator(mir::Terminator::BranchIf { condition,
                                                    then_block: then_id,
                                                    else_block: else_id },
                        *span,
                        lowered);

    let outer_locals = self.locals.clone();
    self.switch_to(then_id);
    self.lower_value_block_return(then_block, return_span, lowered);
    let then_end = self.current_block;

    self.locals.clone_from(&outer_locals);
    self.switch_to(else_id);
    self.lower_value_block_return(else_block, return_span, lowered);
    let else_end = self.current_block;
    self.locals = outer_locals;

    if !self.block_is_terminated(then_end, lowered) || !self.block_is_terminated(else_end, lowered)
    {
      self.report(DiagnosticCode::MissingReturn,
                  "if expression branches must terminate with their value",
                  *span);
    }
    self.switch_to(else_end);
  }

  fn lower_block_statements(&mut self, block: &Block, lowered: &mut mir::Function)
  {
    let mut declared_here = std::collections::HashSet::new();
    for statement in &block.statements
    {
      if self.current_is_terminated(lowered)
      {
        break;
      }
      if let Statement::Let { local, .. } = statement &&
         declared_here.insert(local.name.clone())
      {
        self.locals.remove(&local.name);
      }
      self.lower_statement(statement, lowered);
    }
    if let Some(tail) = &block.tail &&
       !self.current_is_terminated(lowered)
    {
      self.unsupported_expression(tail);
    }
  }

  fn lower_value_block_return(&mut self, block: &Block, return_span: Span, lowered: &mut mir::Function)
  {
    for statement in &block.statements
    {
      if self.current_is_terminated(lowered)
      {
        return;
      }
      self.lower_statement(statement, lowered);
    }
    if !self.current_is_terminated(lowered)
    {
      self.lower_return(block.tail.as_deref(), return_span, lowered);
    }
  }

  fn block_is_terminated(&self, id: mir::BlockId, lowered: &mir::Function) -> bool
  {
    lowered.blocks
           .iter()
           .find(|block| block.id == id)
           .is_some_and(|block| block.terminator.is_some())
  }
}
