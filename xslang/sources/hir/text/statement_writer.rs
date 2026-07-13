/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use super::{
  DesugaredStatement, Statement, mutability_name, type_name, write_block, write_desugared_block,
  write_desugared_expression, write_expression,
};

pub(super) fn write_statement(output: &mut String, statement: &Statement, indent: usize)
{
  let pad = "  ".repeat(indent);
  match statement
  {
    Statement::Let { local,
                     initializer, } =>
    {
      let _ = writeln!(output, "{pad}let {}", local.name);
      let _ = writeln!(output, "{pad}  type {}", type_name(&local.ty));
      let _ = writeln!(output, "{pad}  mutability {}", mutability_name(local.mutable));
      if let Some(initializer) = initializer
      {
        let _ = writeln!(output, "{pad}  initializer");
        write_expression(output, initializer, indent + 2);
      }
    }
    Statement::Expr(expression) =>
    {
      let _ = writeln!(output, "{pad}expression");
      write_expression(output, expression, indent + 1);
    }
    Statement::Return { value, .. } =>
    {
      let _ = writeln!(output, "{pad}return");
      if let Some(value) = value
      {
        write_expression(output, value, indent + 1);
      }
    }
    Statement::If { condition,
                    then_block,
                    else_block,
                    .. } =>
    {
      write_conditional(output, condition, then_block, else_block.as_ref(), indent);
    }
    Statement::While { condition,
                       body,
                       .. } =>
    {
      let _ = writeln!(output, "{pad}while");
      let _ = writeln!(output, "{pad}  condition");
      write_expression(output, condition, indent + 2);
      let _ = writeln!(output, "{pad}  body");
      write_block(output, body, indent + 2);
    }
    Statement::Break { .. } =>
    {
      let _ = writeln!(output, "{pad}break");
    }
    Statement::Continue { .. } =>
    {
      let _ = writeln!(output, "{pad}continue");
    }
    Statement::Panic { .. } =>
    {
      let _ = writeln!(output, "{pad}panic");
    }
  }
}

fn write_conditional(output: &mut String,
                     condition: &super::Expression,
                     then_block: &super::Block,
                     else_block: Option<&super::Block>,
                     indent: usize)
{
  let pad = "  ".repeat(indent);
  let _ = writeln!(output, "{pad}if");
  let _ = writeln!(output, "{pad}  condition");
  write_expression(output, condition, indent + 2);
  let _ = writeln!(output, "{pad}  then");
  write_block(output, then_block, indent + 2);
  if let Some(else_block) = else_block
  {
    let _ = writeln!(output, "{pad}  else");
    write_block(output, else_block, indent + 2);
  }
}

pub(super) fn write_desugared_statement(output: &mut String, statement: &DesugaredStatement, indent: usize)
{
  let pad = "  ".repeat(indent);
  match statement
  {
    DesugaredStatement::Let { local,
                              initializer, } =>
    {
      let _ = writeln!(output, "{pad}let {}", local.name);
      let _ = writeln!(output, "{pad}  type {}", type_name(&local.ty));
      let _ = writeln!(output, "{pad}  mutability {}", mutability_name(local.mutable));
      if let Some(initializer) = initializer
      {
        let _ = writeln!(output, "{pad}  initializer");
        write_desugared_expression(output, initializer, indent + 2);
      }
    }
    DesugaredStatement::Expr(expression) =>
    {
      let _ = writeln!(output, "{pad}expression");
      write_desugared_expression(output, expression, indent + 1);
    }
    DesugaredStatement::Return { value, .. } =>
    {
      let _ = writeln!(output, "{pad}return");
      if let Some(value) = value
      {
        write_desugared_expression(output, value, indent + 1);
      }
    }
    DesugaredStatement::If { condition,
                             then_block,
                             else_block,
                             .. } =>
    {
      let _ = writeln!(output, "{pad}if");
      let _ = writeln!(output, "{pad}  condition");
      write_desugared_expression(output, condition, indent + 2);
      let _ = writeln!(output, "{pad}  then");
      write_desugared_block(output, then_block, indent + 2);
      if let Some(else_block) = else_block
      {
        let _ = writeln!(output, "{pad}  else");
        write_desugared_block(output, else_block, indent + 2);
      }
    }
    DesugaredStatement::While { condition,
                                body,
                                .. } =>
    {
      let _ = writeln!(output, "{pad}while");
      let _ = writeln!(output, "{pad}  condition");
      write_desugared_expression(output, condition, indent + 2);
      let _ = writeln!(output, "{pad}  body");
      write_desugared_block(output, body, indent + 2);
    }
    DesugaredStatement::Break { .. } =>
    {
      let _ = writeln!(output, "{pad}break");
    }
    DesugaredStatement::Continue { .. } =>
    {
      let _ = writeln!(output, "{pad}continue");
    }
    DesugaredStatement::Panic { .. } =>
    {
      let _ = writeln!(output, "{pad}panic");
    }
  }
}
