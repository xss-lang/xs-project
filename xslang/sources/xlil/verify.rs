/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashSet;

use crate::xlil::{Function, Instruction, Module, Terminator, ValueId};

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum DiagnosticCode
{
  EmptyModuleName,
  EmptyFunctionName,
  DuplicateFunctionName,
  DeclarationHasBody,
  DefinitionHasNoBlocks,
  DuplicateBlockId,
  EmptyBlockLabel,
  MissingTerminator,
  InstructionResultUnknown,
  ReturnValueUnknown,
  ReturnValueTypeMismatch,
  VoidReturnValue,
  NonVoidReturnMissingValue,
  BranchTargetUnknown,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Diagnostic
{
  pub code: DiagnosticCode,
  pub message: String,
}

#[must_use]
pub fn verify_module(module: &Module) -> Vec<Diagnostic>
{
  let mut verifier = Verifier::default();
  verifier.module(module);
  verifier.diagnostics
}

#[derive(Default)]
struct Verifier
{
  diagnostics: Vec<Diagnostic>,
}

impl Verifier
{
  fn module(&mut self, module: &Module)
  {
    if module.name.is_empty()
    {
      self.report(DiagnosticCode::EmptyModuleName, "XLIL module name must not be empty");
    }
    let mut names = HashSet::new();
    for function in &module.functions
    {
      if !names.insert(function.name.as_str())
      {
        self.report(DiagnosticCode::DuplicateFunctionName,
                    "XLIL function names must be unique");
      }
      self.function(function);
    }
  }

  fn function(&mut self, function: &Function)
  {
    if function.name.is_empty()
    {
      self.report(DiagnosticCode::EmptyFunctionName,
                  "XLIL function name must not be empty");
    }
    if !function.is_definition && !function.blocks.is_empty()
    {
      self.report(DiagnosticCode::DeclarationHasBody,
                  "XLIL function declaration must not contain blocks");
    }
    if function.is_definition && function.blocks.is_empty()
    {
      self.report(DiagnosticCode::DefinitionHasNoBlocks,
                  "XLIL function definition must contain at least one block");
    }
    let mut blocks = HashSet::new();
    for block in &function.blocks
    {
      if !blocks.insert(block.id)
      {
        self.report(DiagnosticCode::DuplicateBlockId,
                    "XLIL block ids must be unique within a function");
      }
      if block.label.is_empty()
      {
        self.report(DiagnosticCode::EmptyBlockLabel, "XLIL block label must not be empty");
      }
      for instruction in &block.instructions
      {
        self.instruction(function, instruction);
      }
      self.terminator(function, block.terminator.as_ref(), &blocks);
    }
  }

  fn instruction(&mut self, function: &Function, instruction: &Instruction)
  {
    match *instruction
    {
      Instruction::ConstI64 { result, .. } =>
      {
        if value_type(function, result).is_none()
        {
          self.report(DiagnosticCode::InstructionResultUnknown,
                      "XLIL instruction result must reference a declared value");
        }
      }
    }
  }

  fn terminator(&mut self, function: &Function, terminator: Option<&Terminator>, blocks: &HashSet<crate::xlil::BlockId>)
  {
    let Some(terminator) = terminator
    else
    {
      self.report(DiagnosticCode::MissingTerminator, "XLIL block must have a terminator");
      return;
    };
    match terminator
    {
      Terminator::Return(Some(value)) => self.return_value(function, *value),
      Terminator::Return(None) =>
      {
        if function.return_type.kind != crate::xlil::TypeKind::Void
        {
          self.report(DiagnosticCode::NonVoidReturnMissingValue,
                      "non-void XLIL function must return a value");
        }
      }
      Terminator::Branch(target) =>
      {
        if !blocks.contains(target)
        {
          self.report(DiagnosticCode::BranchTargetUnknown,
                      "XLIL branch target must reference an existing block");
        }
      }
    }
  }

  fn return_value(&mut self, function: &Function, value: ValueId)
  {
    let Some(value_type) = value_type(function, value)
    else
    {
      self.report(DiagnosticCode::ReturnValueUnknown,
                  "XLIL return references an unknown value");
      return;
    };
    if function.return_type.kind == crate::xlil::TypeKind::Void
    {
      self.report(DiagnosticCode::VoidReturnValue,
                  "void XLIL function cannot return a value");
      return;
    }
    if value_type != function.return_type
    {
      self.report(DiagnosticCode::ReturnValueTypeMismatch,
                  "XLIL return value type must match function return type");
    }
  }

  fn report(&mut self, code: DiagnosticCode, message: &str)
  {
    self.diagnostics.push(Diagnostic { code,
                                       message: message.to_string() });
  }
}

fn value_type(function: &Function, value: ValueId) -> Option<crate::xlil::Type>
{
  function.values
          .iter()
          .find(|candidate| candidate.id == value)
          .map(|value| value.value_type)
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::xlil::{Block, BlockId, Function, Instruction, Module, Terminator, Type, Value, ValueId};

  fn valid_module() -> Module
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("xs$App$Main", Type::VOID, vec![]);
    let block = function.append_block("entry");
    assert!(function.set_return(block, None));
    module.add_function(function);
    module
  }

  #[test]
  fn accepts_valid_void_function()
  {
    assert!(verify_module(&valid_module()).is_empty());
  }

  #[test]
  fn rejects_duplicate_function_names()
  {
    let mut module = Module::new("App");
    module.add_function(Function::declaration("same", Type::VOID, vec![]));
    module.add_function(Function::declaration("same", Type::VOID, vec![]));

    let diagnostics = verify_module(&module);

    assert!(diagnostics.iter()
                       .any(|diagnostic| diagnostic.code == DiagnosticCode::DuplicateFunctionName));
  }

  #[test]
  fn rejects_definition_without_blocks()
  {
    let mut module = Module::new("App");
    module.add_function(Function::definition("empty", Type::VOID, vec![]));

    let diagnostics = verify_module(&module);

    assert_eq!(diagnostics[0].code, DiagnosticCode::DefinitionHasNoBlocks);
  }

  #[test]
  fn rejects_missing_block_terminator()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("bad", Type::VOID, vec![]);
    function.append_block("entry");
    module.add_function(function);

    let diagnostics = verify_module(&module);

    assert_eq!(diagnostics[0].code, DiagnosticCode::MissingTerminator);
  }

  #[test]
  fn rejects_unknown_return_value()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("bad", Type::I64, vec![]);
    function.blocks.push(Block { id: BlockId(0),
                                 label: "entry".to_string(),
                                 instructions: vec![],
                                 terminator: Some(Terminator::Return(Some(ValueId(99)))) });
    module.add_function(function);

    let diagnostics = verify_module(&module);

    assert_eq!(diagnostics[0].code, DiagnosticCode::ReturnValueUnknown);
  }

  #[test]
  fn rejects_instruction_result_without_value_record()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("bad", Type::VOID, vec![]);
    function.blocks.push(Block { id: BlockId(0),
                                 label: "entry".to_string(),
                                 instructions: vec![Instruction::ConstI64 { result: ValueId(0),
                                                                            value: 42 }],
                                 terminator: Some(Terminator::Return(None)) });
    module.add_function(function);

    let diagnostics = verify_module(&module);

    assert_eq!(diagnostics[0].code, DiagnosticCode::InstructionResultUnknown);
  }

  #[test]
  fn rejects_return_type_mismatch()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("bad", Type::VOID, vec![]);
    function.values.push(Value { id: ValueId(0),
                                 value_type: Type::I64 });
    function.blocks.push(Block { id: BlockId(0),
                                 label: "entry".to_string(),
                                 instructions: vec![],
                                 terminator: Some(Terminator::Return(Some(ValueId(0)))) });
    module.add_function(function);

    let diagnostics = verify_module(&module);

    assert_eq!(diagnostics[0].code, DiagnosticCode::VoidReturnValue);
  }

  #[test]
  fn rejects_unknown_branch_target()
  {
    let mut module = Module::new("App");
    let mut function = Function::definition("bad", Type::VOID, vec![]);
    function.blocks.push(Block { id: BlockId(0),
                                 label: "entry".to_string(),
                                 instructions: vec![],
                                 terminator: Some(Terminator::Branch(BlockId(99))) });
    module.add_function(function);

    let diagnostics = verify_module(&module);

    assert_eq!(diagnostics[0].code, DiagnosticCode::BranchTargetUnknown);
  }
}
