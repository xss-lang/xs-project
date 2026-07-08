/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::{self, Write};

use crate::xlil::{Block, Function, Instruction, Module, Terminator, ValueId, type_name};

pub fn write_module(module: &Module, output: &mut impl Write) -> fmt::Result
{
  writeln!(output, ".xlil version 0")?;
  writeln!(output, ".xlil module {}", module.name)?;
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
    write!(output, ".func ")?;
  }
  else
  {
    write!(output, ".extern ")?;
  }
  write_signature(function, output)?;
  if !function.is_definition
  {
    return Ok(());
  }
  for block in &function.blocks
  {
    write_block(block, output)?;
  }
  writeln!(output, ".end")
}

fn write_signature(function: &Function, output: &mut impl Write) -> fmt::Result
{
  write!(output, "{} : (", function.name)?;
  for (index, parameter) in function.parameters.iter().enumerate()
  {
    if index != 0
    {
      write!(output, ", ")?;
    }
    write!(output, "{}", type_name(*parameter))?;
  }
  writeln!(output, ") -> {}", type_name(function.return_type))
}

fn write_block(block: &Block, output: &mut impl Write) -> fmt::Result
{
  writeln!(output, "bb{}.{}:", block.id.0, block.label)?;
  for instruction in &block.instructions
  {
    write_instruction(instruction, output)?;
  }
  match block.terminator
  {
    Some(Terminator::Return(value)) => write_return(value, output),
    Some(Terminator::Branch(target)) => writeln!(output, "  br bb{}", target.0),
    None => writeln!(output, "  .missing_terminator"),
  }
}

fn write_instruction(instruction: &Instruction, output: &mut impl Write) -> fmt::Result
{
  match *instruction
  {
    Instruction::ConstI64 { result,
                            value, } => writeln!(output, "  %{}:i64 = const {}", result.0, value),
    Instruction::Call { result,
                        ref function,
                        ref arguments,
                        return_type, } =>
    {
      if let Some(result) = result
      {
        write!(output, "  %{}:{} = ", result.0, type_name(return_type))?;
      }
      else
      {
        write!(output, "  ")?;
      }
      write!(output, "call {function}(")?;
      for (index, argument) in arguments.iter().enumerate()
      {
        if index != 0
        {
          write!(output, ", ")?;
        }
        write!(output, "%{}", argument.0)?;
      }
      writeln!(output, ")")
    }
  }
}

fn write_return(value: Option<ValueId>, output: &mut impl Write) -> fmt::Result
{
  match value
  {
    Some(value) => writeln!(output, "  ret %{}", value.0),
    None => writeln!(output, "  ret"),
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
               ".xlil version 0\n.xlil module App\n.extern xs$App$External : (i64) -> i64\n");
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
               ".xlil version 0\n.xlil module App\n.func xs$App$Main : () -> void\nbb0.entry:\n  ret\n.end\n");
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
               ".xlil version 0\n.xlil module App\n.func xs$App$Value : () -> i64\nbb0.entry:\n  %0:i64 = const 42\n  ret %0\n.end\n");
  }

  #[test]
  fn writes_branch_terminator()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("xs$App$Branch", Type::VOID, vec![]);
    let entry = function.append_block("entry");
    let exit = function.append_block("exit");
    assert!(function.set_branch(entry, exit));
    assert!(function.set_return(exit, None));
    module.add_function(function);

    assert_eq!(module_to_string(&module),
               ".xlil version 0\n.xlil module App\n.func xs$App$Branch : () -> void\nbb0.entry:\n  br \
                bb1\nbb1.exit:\n  ret\n.end\n");
  }

  #[test]
  fn writes_call_instruction()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("xs$App$Call", Type::I64, vec![]);
    let entry = function.append_block("entry");
    let argument = function.add_const_i64(entry, 7).expect("const should be added");
    let result = function.add_call(entry, "xs$App$Callee", vec![argument], Type::I64)
                         .expect("call should be added");
    assert!(function.set_return(entry, result));
    module.add_function(function);

    assert_eq!(module_to_string(&module),
               ".xlil version 0\n.xlil module App\n.func xs$App$Call : () -> i64\nbb0.entry:\n  %0:i64 = const 7\n  \
                %1:i64 = call xs$App$Callee(%0)\n  ret %1\n.end\n");
  }
}
