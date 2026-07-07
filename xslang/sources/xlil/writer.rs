/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::{self, Write};

use crate::xlil::{Block, Function, Instruction, Module, Terminator, ValueId, type_name};

pub fn write_module(module: &Module, output: &mut impl Write) -> fmt::Result
{
  writeln!(output, "xlil module {}", module.name)?;
  for function in &module.functions
  {
    write_function(function, output)?;
  }
  Ok(())
}

#[must_use]
pub fn module_to_string(module: &Module) -> String
{
  let mut output = String::new();
  write_module(module, &mut output).expect("writing XLIL to String cannot fail");
  output
}

fn write_function(function: &Function, output: &mut impl Write) -> fmt::Result
{
  if function.is_definition
  {
    write!(output, "define ")?;
  }
  else
  {
    write!(output, "declare ")?;
  }
  write_signature(function, output)?;
  if !function.is_definition
  {
    writeln!(output)?;
    return Ok(());
  }
  writeln!(output, " {{")?;
  for block in &function.blocks
  {
    write_block(block, output)?;
  }
  writeln!(output, "}}")
}

fn write_signature(function: &Function, output: &mut impl Write) -> fmt::Result
{
  write!(output, "{} {}(", type_name(function.return_type), function.name)?;
  for (index, parameter) in function.parameters.iter().enumerate()
  {
    if index != 0
    {
      write!(output, ", ")?;
    }
    write!(output, "{}", type_name(*parameter))?;
  }
  write!(output, ")")
}

fn write_block(block: &Block, output: &mut impl Write) -> fmt::Result
{
  writeln!(output, "bb{} {}:", block.id.0, block.label)?;
  for instruction in &block.instructions
  {
    write_instruction(instruction, output)?;
  }
  match block.terminator
  {
    Some(Terminator::Return(value)) => write_return(value, output),
    None => writeln!(output, "  <missing terminator>"),
  }
}

fn write_instruction(instruction: &Instruction, output: &mut impl Write) -> fmt::Result
{
  match *instruction
  {
    Instruction::ConstI64 { result,
                            value, } => writeln!(output, "  r{} = const.i64 {}", result.0, value),
  }
}

fn write_return(value: Option<ValueId>, output: &mut impl Write) -> fmt::Result
{
  match value
  {
    Some(value) => writeln!(output, "  return r{}", value.0),
    None => writeln!(output, "  return"),
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::xlil::{Function, Module, Type};

  #[test]
  fn writes_function_declaration()
  {
    let mut module = Module::new("App");
    module.add_function(Function::declaration("xs$App$External", Type::I64, vec![Type::I64]));

    assert_eq!(module_to_string(&module),
               "xlil module App\ndeclare i64 xs$App$External(i64)\n");
  }

  #[test]
  fn writes_void_function_definition()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("xs$App$Main", Type::VOID, vec![]);
    let entry = function.append_block("entry");
    assert!(function.set_return(entry, None));
    module.add_function(function);

    assert_eq!(module_to_string(&module),
               "xlil module App\ndefine void xs$App$Main() {\nbb0 entry:\n  return\n}\n");
  }

  #[test]
  fn writes_const_i64_and_return_value()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("xs$App$Value", Type::I64, vec![]);
    let entry = function.append_block("entry");
    let value = function.add_const_i64(entry, 42);
    assert!(function.set_return(entry, value));
    module.add_function(function);

    assert_eq!(module_to_string(&module),
               "xlil module App\ndefine i64 xs$App$Value() {\nbb0 entry:\n  r0 = const.i64 42\n  return r0\n}\n");
  }
}
