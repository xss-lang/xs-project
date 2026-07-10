/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use crate::mir::{BasicBlock, Function, LocalId, Parameter, Statement, Terminator};
use crate::xlil::type_name;

#[must_use]
pub fn function_to_xmir(function: &Function) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, ".xmir version 0");
  let _ = writeln!(output, "function {}", function.name);
  let _ = writeln!(output, "returns {}", type_name(function.return_type));
  write_parameters(&mut output, function);
  write_locals(&mut output, function);
  write_blocks(&mut output, function);
  let _ = writeln!(output, ".program end");
  output
}

fn write_parameters(output: &mut String, function: &Function)
{
  if function.parameters.is_empty()
  {
    return;
  }
  let _ = writeln!(output);
  let _ = writeln!(output, "parameters");
  for parameter in &function.parameters
  {
    write_parameter(output, parameter);
  }
  let _ = writeln!(output, ".end");
}

fn write_parameter(output: &mut String, parameter: &Parameter)
{
  let _ = writeln!(output, "  parameter {}", parameter.name);
  let _ = writeln!(output, "    local {}", parameter.local.0);
  let _ = writeln!(output, "    type {}", type_name(parameter.value_type));
}

fn write_locals(output: &mut String, function: &Function)
{
  if function.locals.is_empty()
  {
    return;
  }
  let _ = writeln!(output);
  let _ = writeln!(output, "locals");
  for local in &function.locals
  {
    let mutability = if local.mutable
    {
      "mutable"
    }
    else
    {
      "immutable"
    };
    let _ = writeln!(output, "  local {}", local.id.0);
    let _ = writeln!(output, "    name {}", local.name);
    if let Some(value_type) = local.value_type
    {
      let _ = writeln!(output, "    type {}", type_name(value_type));
    }
    let _ = writeln!(output, "    mutability {mutability}");
  }
  let _ = writeln!(output, ".end");
}

fn write_blocks(output: &mut String, function: &Function)
{
  if function.blocks.is_empty()
  {
    return;
  }
  let _ = writeln!(output);
  let _ = writeln!(output, "control_flow");
  for block in &function.blocks
  {
    write_block(output, block);
  }
  let _ = writeln!(output, ".end");
}

fn write_block(output: &mut String, block: &BasicBlock)
{
  let _ = writeln!(output, "  block {}", block.id.0);
  if !block.statements.is_empty()
  {
    let _ = writeln!(output, "    statements");
    for statement in &block.statements
    {
      write_statement(output, statement);
    }
  }
  match &block.terminator
  {
    Some(terminator) => write_terminator(output, terminator),
    None =>
    {
      let _ = writeln!(output, "    terminator missing");
    }
  }
}

fn write_statement(output: &mut String, statement: &Statement)
{
  match statement
  {
    Statement::Use { local, .. } => write_local_statement(output, "use", *local),
    Statement::Move { local, .. } => write_local_statement(output, "move", *local),
    Statement::BorrowShared { local, .. } => write_local_statement(output, "borrow shared", *local),
    Statement::BorrowMutable { local, .. } => write_local_statement(output, "borrow mutable", *local),
    Statement::EndBorrow { local, .. } => write_local_statement(output, "borrow end", *local),
    Statement::ConstI64 { local,
                          value,
                          .. } =>
    {
      let _ = writeln!(output, "      statement const.i64");
      let _ = writeln!(output, "        target local {}", local.0);
      let _ = writeln!(output, "        value {value}");
    }
    Statement::ConstBool { local,
                           value,
                           .. } =>
    {
      let _ = writeln!(output, "      statement const.bool");
      let _ = writeln!(output, "        target local {}", local.0);
      let _ = writeln!(output, "        value {value}");
    }
    Statement::AddI64 { result,
                        left,
                        right,
                        .. } => write_i64_binary(output, "add.i64", *result, *left, *right),
    Statement::SubI64 { result,
                        left,
                        right,
                        .. } => write_i64_binary(output, "sub.i64", *result, *left, *right),
    Statement::MulI64 { result,
                        left,
                        right,
                        .. } => write_i64_binary(output, "mul.i64", *result, *left, *right),
    Statement::EqI64 { result,
                       left,
                       right,
                       .. } => write_i64_binary(output, "eq.i64", *result, *left, *right),
    Statement::Call { result,
                      function,
                      arguments,
                      return_type,
                      .. } =>
    {
      let _ = writeln!(output, "      statement call");
      let _ = writeln!(output, "        function {function}");
      let _ = writeln!(output, "        returns {}", type_name(*return_type));
      write_call_result(output, *result);
      for argument in arguments
      {
        let _ = writeln!(output, "        argument local {}", argument.0);
      }
    }
    Statement::Drop { local, .. } => write_local_statement(output, "drop", *local),
  }
}

fn write_i64_binary(output: &mut String, name: &str, result: LocalId, left: LocalId, right: LocalId)
{
  let _ = writeln!(output, "      statement {name}");
  let _ = writeln!(output, "        result local {}", result.0);
  let _ = writeln!(output, "        left local {}", left.0);
  let _ = writeln!(output, "        right local {}", right.0);
}

fn write_call_result(output: &mut String, result: Option<LocalId>)
{
  if let Some(result) = result
  {
    let _ = writeln!(output, "        result local {}", result.0);
  }
  else
  {
    let _ = writeln!(output, "        result discard");
  }
}

fn write_local_statement(output: &mut String, name: &str, local: LocalId)
{
  let _ = writeln!(output, "      statement {name}");
  let _ = writeln!(output, "        local {}", local.0);
}

fn write_terminator(output: &mut String, terminator: &Terminator)
{
  match terminator
  {
    Terminator::Return(Some(local)) =>
    {
      let _ = writeln!(output, "    terminator return");
      let _ = writeln!(output, "      value local {}", local.0);
    }
    Terminator::Return(None) =>
    {
      let _ = writeln!(output, "    terminator return");
    }
    Terminator::Goto(block) =>
    {
      let _ = writeln!(output, "    terminator goto");
      let _ = writeln!(output, "      target block {}", block.0);
    }
    Terminator::BranchIf { condition,
                           then_block,
                           else_block, } =>
    {
      let _ = writeln!(output, "    terminator branch_if");
      let _ = writeln!(output, "      condition local {}", condition.0);
      let _ = writeln!(output, "      then block {}", then_block.0);
      let _ = writeln!(output, "      else block {}", else_block.0);
    }
    Terminator::Unreachable =>
    {
      let _ = writeln!(output, "    terminator unreachable");
    }
  }
}
