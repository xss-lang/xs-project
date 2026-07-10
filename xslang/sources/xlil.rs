/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

pub mod lowering;
pub mod parser;
pub mod verify;
pub mod writer;

pub const SUPPORTED_XLIL_VERSION: u32 = 0;

#[must_use]
pub const fn is_supported_xlil_version(version: u32) -> bool
{
  matches!(version, SUPPORTED_XLIL_VERSION)
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum TypeKind
{
  Void,
  Bool,
  U8,
  I8,
  U16,
  I16,
  U32,
  I32,
  U64,
  I64,
  U128,
  I128,
  F16,
  F32,
  F64,
  F128,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Type
{
  pub kind: TypeKind,
}

impl Type
{
  pub const VOID: Self = Self { kind: TypeKind::Void };
  pub const BOOL: Self = Self { kind: TypeKind::Bool };
  pub const I64: Self = Self { kind: TypeKind::I64 };
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct ValueId(pub u32);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct BlockId(pub u32);

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Value
{
  pub id: ValueId,
  pub value_type: Type,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Instruction
{
  ConstI64
  {
    result: ValueId, value: i64
  },
  ConstBool
  {
    result: ValueId, value: bool
  },
  AddI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  SubI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  MulI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  EqI64
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  Call
  {
    result: Option<ValueId>,
    function: String,
    arguments: Vec<ValueId>,
    return_type: Type,
  },
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Terminator
{
  Return(Option<ValueId>),
  Branch(BlockId),
  BranchIf
  {
    condition: ValueId,
    then_block: BlockId,
    else_block: BlockId,
  },
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Block
{
  pub id: BlockId,
  pub label: String,
  pub instructions: Vec<Instruction>,
  pub terminator: Option<Terminator>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Function
{
  pub name: String,
  pub return_type: Type,
  pub parameters: Vec<Type>,
  pub values: Vec<Value>,
  pub blocks: Vec<Block>,
  pub is_definition: bool,
}

impl Function
{
  #[must_use]
  pub fn declaration(name: impl Into<String>, return_type: Type, parameters: Vec<Type>) -> Self
  {
    Self { name: name.into(),
           return_type,
           parameters,
           values: vec![],
           blocks: vec![],
           is_definition: false }
  }

  #[must_use]
  pub fn definition(name: impl Into<String>, return_type: Type, parameters: Vec<Type>) -> Self
  {
    let values = parameters.iter()
                           .enumerate()
                           .map(|(index, value_type)| Value { id: ValueId(index as u32),
                                                              value_type: *value_type })
                           .collect();
    Self { name: name.into(),
           return_type,
           parameters,
           values,
           blocks: vec![],
           is_definition: true }
  }

  #[must_use]
  pub fn parameter_value(&self, parameter: usize) -> Option<ValueId>
  {
    self.parameters.get(parameter)?;
    Some(ValueId(parameter as u32))
  }

  pub fn append_block(&mut self, label: impl Into<String>) -> BlockId
  {
    let id = BlockId(self.blocks.len() as u32);
    self.blocks.push(Block { id,
                             label: label.into(),
                             instructions: vec![],
                             terminator: None });
    id
  }

  pub fn add_const_i64(&mut self, block: BlockId, value: i64) -> Option<ValueId>
  {
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::I64 });
    self.block_mut(block)?.instructions.push(Instruction::ConstI64 { result,
                                                                     value });
    Some(result)
  }

  pub fn add_const_bool(&mut self, block: BlockId, value: bool) -> Option<ValueId>
  {
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::BOOL });
    self.block_mut(block)?
        .instructions
        .push(Instruction::ConstBool { result,
                                       value });
    Some(result)
  }

  pub fn add_i64(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I64) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I64)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::I64 });
    self.block_mut(block)?.instructions.push(Instruction::AddI64 { result,
                                                                   left,
                                                                   right });
    Some(result)
  }

  pub fn sub_i64(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I64) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I64)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::I64 });
    self.block_mut(block)?.instructions.push(Instruction::SubI64 { result,
                                                                   left,
                                                                   right });
    Some(result)
  }

  pub fn mul_i64(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I64) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I64)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::I64 });
    self.block_mut(block)?.instructions.push(Instruction::MulI64 { result,
                                                                   left,
                                                                   right });
    Some(result)
  }

  pub fn eq_i64(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I64) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I64)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::BOOL });
    self.block_mut(block)?.instructions.push(Instruction::EqI64 { result,
                                                                  left,
                                                                  right });
    Some(result)
  }

  pub fn add_call(&mut self,
                  block: BlockId,
                  function: impl Into<String>,
                  arguments: Vec<ValueId>,
                  return_type: Type)
                  -> Option<Option<ValueId>>
  {
    self.block(block)?;
    if arguments.iter().any(|argument| self.value(*argument).is_none())
    {
      return None;
    }
    let result = if return_type == Type::VOID
    {
      None
    }
    else
    {
      let result = ValueId(self.values.len() as u32);
      self.values.push(Value { id: result,
                               value_type: return_type });
      Some(result)
    };
    self.block_mut(block)?.instructions.push(Instruction::Call { result,
                                                                 function: function.into(),
                                                                 arguments,
                                                                 return_type });
    Some(result)
  }

  pub fn set_return(&mut self, block: BlockId, value: Option<ValueId>) -> bool
  {
    let return_type_matches = match value
    {
      Some(value) => self.values
                         .get(value.0 as usize)
                         .is_some_and(|value| value.value_type == self.return_type),
      None => self.return_type == Type::VOID,
    };
    if !return_type_matches
    {
      return false;
    }
    let Some(block) = self.block_mut(block)
    else
    {
      return false;
    };
    if block.terminator.is_some()
    {
      return false;
    }
    block.terminator = Some(Terminator::Return(value));
    true
  }

  pub fn set_branch(&mut self, block: BlockId, target: BlockId) -> bool
  {
    if self.block(target).is_none()
    {
      return false;
    }
    let Some(block) = self.block_mut(block)
    else
    {
      return false;
    };
    if block.terminator.is_some()
    {
      return false;
    }
    block.terminator = Some(Terminator::Branch(target));
    true
  }

  pub fn set_branch_if(&mut self, block: BlockId, condition: ValueId, then_block: BlockId, else_block: BlockId)
                       -> bool
  {
    if self.block(then_block).is_none() || self.block(else_block).is_none()
    {
      return false;
    }
    if !self.value(condition)
            .is_some_and(|value| value.value_type == Type::BOOL)
    {
      return false;
    }
    let Some(block) = self.block_mut(block)
    else
    {
      return false;
    };
    if block.terminator.is_some()
    {
      return false;
    }
    block.terminator = Some(Terminator::BranchIf { condition,
                                                   then_block,
                                                   else_block });
    true
  }

  fn block(&self, block: BlockId) -> Option<&Block>
  {
    self.blocks
        .get(block.0 as usize)
        .filter(|candidate| candidate.id == block)
  }

  fn block_mut(&mut self, block: BlockId) -> Option<&mut Block>
  {
    self.blocks
        .get_mut(block.0 as usize)
        .filter(|candidate| candidate.id == block)
  }

  fn value(&self, value: ValueId) -> Option<&Value>
  {
    self.values
        .get(value.0 as usize)
        .filter(|candidate| candidate.id == value)
  }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Module
{
  pub name: String,
  pub functions: Vec<Function>,
}

impl Module
{
  #[must_use]
  pub fn new(name: impl Into<String>) -> Self
  {
    Self { name: name.into(),
           functions: vec![] }
  }

  pub fn add_function(&mut self, function: Function)
  {
    self.functions.push(function);
  }
}

#[must_use]
pub fn type_name(value_type: Type) -> &'static str
{
  match value_type.kind
  {
    TypeKind::Void => "void",
    TypeKind::Bool => "bool",
    TypeKind::U8 => "u8",
    TypeKind::I8 => "i8",
    TypeKind::U16 => "u16",
    TypeKind::I16 => "i16",
    TypeKind::U32 => "u32",
    TypeKind::I32 => "i32",
    TypeKind::U64 => "u64",
    TypeKind::I64 => "i64",
    TypeKind::U128 => "u128",
    TypeKind::I128 => "i128",
    TypeKind::F16 => "f16",
    TypeKind::F32 => "f32",
    TypeKind::F64 => "f64",
    TypeKind::F128 => "f128",
  }
}

#[must_use]
pub fn type_from_name(name: &str) -> Option<Type>
{
  let kind = match name
  {
    "void" => TypeKind::Void,
    "bool" => TypeKind::Bool,
    "u8" => TypeKind::U8,
    "i8" => TypeKind::I8,
    "u16" => TypeKind::U16,
    "i16" => TypeKind::I16,
    "u32" => TypeKind::U32,
    "i32" => TypeKind::I32,
    "u64" => TypeKind::U64,
    "i64" => TypeKind::I64,
    "u128" => TypeKind::U128,
    "i128" => TypeKind::I128,
    "f16" => TypeKind::F16,
    "f32" => TypeKind::F32,
    "f64" => TypeKind::F64,
    "f128" => TypeKind::F128,
    _ => return None,
  };
  Some(Type { kind })
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn models_function_declaration_signature()
  {
    let function = Function::declaration("xs$App$Main", Type::VOID, vec![Type::I64]);

    assert!(!function.is_definition);
    assert_eq!(function.name, "xs$App$Main");
    assert_eq!(function.parameters, vec![Type::I64]);
  }

  #[test]
  fn definitions_allocate_parameter_values_before_instruction_results()
  {
    let mut function = Function::definition("Identity", Type::I64, vec![Type::I64]);
    let block = function.append_block("entry");

    assert_eq!(function.parameter_value(0), Some(ValueId(0)));
    assert_eq!(function.add_const_i64(block, 42), Some(ValueId(1)));
  }

  #[test]
  fn validates_return_value_type()
  {
    let mut function = Function::definition("Value", Type::I64, vec![]);
    let block = function.append_block("entry");
    let value = function.add_const_i64(block, 42);

    assert!(function.set_return(block, value));
  }

  #[test]
  fn adds_call_instruction_with_result()
  {
    let mut function = Function::definition("Call", Type::I64, vec![]);
    let block = function.append_block("entry");
    let argument = function.add_const_i64(block, 7).expect("const should be added");
    let result = function.add_call(block, "callee", vec![argument], Type::I64)
                         .expect("call should be added");

    assert_eq!(result, Some(ValueId(1)));
    assert!(function.set_return(block, result));
  }

  #[test]
  fn adds_i64_instruction_with_i64_operands()
  {
    let mut function = Function::definition("Add", Type::I64, vec![]);
    let block = function.append_block("entry");
    let left = function.add_const_i64(block, 2).expect("left const should be added");
    let right = function.add_const_i64(block, 3).expect("right const should be added");
    let result = function.add_i64(block, left, right).expect("add should be added");

    assert_eq!(result, ValueId(2));
    assert!(function.set_return(block, Some(result)));
  }

  #[test]
  fn subtracts_i64_instruction_with_i64_operands()
  {
    let mut function = Function::definition("Sub", Type::I64, vec![]);
    let block = function.append_block("entry");
    let left = function.add_const_i64(block, 8).expect("left const should be added");
    let right = function.add_const_i64(block, 3).expect("right const should be added");
    let result = function.sub_i64(block, left, right).expect("sub should be added");

    assert_eq!(result, ValueId(2));
    assert!(function.set_return(block, Some(result)));
  }

  #[test]
  fn multiplies_i64_instruction_with_i64_operands()
  {
    let mut function = Function::definition("Mul", Type::I64, vec![]);
    let block = function.append_block("entry");
    let left = function.add_const_i64(block, 6).expect("left const should be added");
    let right = function.add_const_i64(block, 7).expect("right const should be added");
    let result = function.mul_i64(block, left, right).expect("mul should be added");

    assert_eq!(result, ValueId(2));
    assert!(function.set_return(block, Some(result)));
  }

  #[test]
  fn adds_bool_const_and_i64_equality()
  {
    let mut function = Function::definition("Eq", Type::BOOL, vec![]);
    let block = function.append_block("entry");
    let left = function.add_const_i64(block, 7).expect("left const should be added");
    let right = function.add_const_i64(block, 7).expect("right const should be added");
    let result = function.eq_i64(block, left, right).expect("eq should be added");

    assert_eq!(result, ValueId(2));
    assert!(function.set_return(block, Some(result)));
  }

  #[test]
  fn adds_bool_const_instruction()
  {
    let mut function = Function::definition("Truth", Type::BOOL, vec![]);
    let block = function.append_block("entry");
    let result = function.add_const_bool(block, true)
                         .expect("bool const should be added");

    assert_eq!(result, ValueId(0));
    assert!(function.set_return(block, Some(result)));
  }

  #[test]
  fn sets_conditional_branch_with_bool_condition()
  {
    let mut function = Function::definition("BranchIf", Type::VOID, vec![]);
    let entry = function.append_block("entry");
    let then_block = function.append_block("then");
    let else_block = function.append_block("else");
    let condition = function.add_const_bool(entry, true)
                            .expect("bool const should be added");

    assert!(function.set_branch_if(entry, condition, then_block, else_block));
    assert_eq!(function.blocks[0].terminator,
               Some(Terminator::BranchIf { condition,
                                           then_block,
                                           else_block }));
  }

  #[test]
  fn rejects_void_return_from_non_void_function()
  {
    let mut function = Function::definition("Value", Type::I64, vec![]);
    let block = function.append_block("entry");

    assert!(!function.set_return(block, None));
  }

  #[test]
  fn sets_branch_to_existing_block()
  {
    let mut function = Function::definition("Branch", Type::VOID, vec![]);
    let entry = function.append_block("entry");
    let exit = function.append_block("exit");

    assert!(function.set_branch(entry, exit));
    assert_eq!(function.blocks[0].terminator, Some(Terminator::Branch(exit)));
  }

  #[test]
  fn reports_primitive_type_names()
  {
    assert_eq!(type_name(Type::VOID), "void");
    assert_eq!(type_name(Type::I64), "i64");
  }

  #[test]
  fn parses_primitive_type_names()
  {
    assert_eq!(type_from_name("void"), Some(Type::VOID));
    assert_eq!(type_from_name("i64"), Some(Type::I64));
    assert_eq!(type_from_name("unknown"), None);
  }
}
