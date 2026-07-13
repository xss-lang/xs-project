/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{I32BinaryOperation, I64BinaryOperation, I64ComparisonOperation};

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
  pub const I32: Self = Self { kind: TypeKind::I32 };
  pub const I64: Self = Self { kind: TypeKind::I64 };
  pub const F32: Self = Self { kind: TypeKind::F32 };
  pub const F64: Self = Self { kind: TypeKind::F64 };
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct ValueId(pub u32);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct BlockId(pub u32);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct SlotId(pub u32);

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Value
{
  pub id: ValueId,
  pub value_type: Type,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Slot
{
  pub id: SlotId,
  pub value_type: Type,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Instruction
{
  ConstI64
  {
    result: ValueId, value: i64
  },
  ConstI32
  {
    result: ValueId, value: i32
  },
  ConstF32
  {
    result: ValueId, bits: u32
  },
  ConstF64
  {
    result: ValueId, bits: u64
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
  BinaryI64
  {
    operation: I64BinaryOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  CompareI64
  {
    operation: I64ComparisonOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  AddI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  SubI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  MulI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  BinaryI32
  {
    operation: I32BinaryOperation,
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  EqI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  LtI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  LeI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  GtI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  GeI32
  {
    result: ValueId,
    left: ValueId,
    right: ValueId,
  },
  NotBool
  {
    result: ValueId, operand: ValueId
  },
  Call
  {
    result: Option<ValueId>,
    function: String,
    arguments: Vec<ValueId>,
    return_type: Type,
  },
  Load
  {
    result: ValueId, slot: SlotId
  },
  Store
  {
    slot: SlotId, value: ValueId
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
  Panic,
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
  pub slots: Vec<Slot>,
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
           slots: vec![],
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
           slots: vec![],
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

  pub fn add_const_i32(&mut self, block: BlockId, value: i32) -> Option<ValueId>
  {
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::I32 });
    self.block_mut(block)?.instructions.push(Instruction::ConstI32 { result,
                                                                     value });
    Some(result)
  }

  pub fn add_const_f32_bits(&mut self, block: BlockId, bits: u32) -> Option<ValueId>
  {
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::F32 });
    self.block_mut(block)?.instructions.push(Instruction::ConstF32 { result,
                                                                     bits });
    Some(result)
  }

  pub fn add_const_f64_bits(&mut self, block: BlockId, bits: u64) -> Option<ValueId>
  {
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::F64 });
    self.block_mut(block)?.instructions.push(Instruction::ConstF64 { result,
                                                                     bits });
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

  pub fn add_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::I32, I32Op::Add)
  }

  pub fn sub_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::I32, I32Op::Sub)
  }

  pub fn mul_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::I32, I32Op::Mul)
  }

  pub fn binary_i32(&mut self,
                    block: BlockId,
                    left: ValueId,
                    right: ValueId,
                    operation: I32BinaryOperation)
                    -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I32) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I32)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: Type::I32 });
    self.block_mut(block)?
        .instructions
        .push(Instruction::BinaryI32 { operation,
                                       result,
                                       left,
                                       right });
    Some(result)
  }

  pub fn eq_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::BOOL, I32Op::Eq)
  }

  pub fn lt_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::BOOL, I32Op::Lt)
  }

  pub fn le_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::BOOL, I32Op::Le)
  }

  pub fn gt_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::BOOL, I32Op::Gt)
  }

  pub fn ge_i32(&mut self, block: BlockId, left: ValueId, right: ValueId) -> Option<ValueId>
  {
    self.add_i32_like(block, left, right, Type::BOOL, I32Op::Ge)
  }

  fn add_i32_like(&mut self,
                  block: BlockId,
                  left: ValueId,
                  right: ValueId,
                  result_type: Type,
                  op: I32Op)
                  -> Option<ValueId>
  {
    self.block(block)?;
    if !self.value(left).is_some_and(|value| value.value_type == Type::I32) ||
       !self.value(right).is_some_and(|value| value.value_type == Type::I32)
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: result_type });
    self.block_mut(block)?
        .instructions
        .push(i32_instruction(op, result, left, right));
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

  pub fn add_slot(&mut self, value_type: Type) -> Option<SlotId>
  {
    if value_type == Type::VOID
    {
      return None;
    }
    let id = SlotId(self.slots.len() as u32);
    self.slots.push(Slot { id,
                           value_type });
    Some(id)
  }

  pub fn add_load(&mut self, block: BlockId, slot: SlotId) -> Option<ValueId>
  {
    let value_type = self.slot(slot)?.value_type;
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type });
    self.block_mut(block)?.instructions.push(Instruction::Load { result,
                                                                 slot });
    Some(result)
  }

  pub fn add_store(&mut self, block: BlockId, slot: SlotId, value: ValueId) -> bool
  {
    let Some(slot_type) = self.slot(slot).map(|slot| slot.value_type)
    else
    {
      return false;
    };
    if !self.value(value).is_some_and(|value| value.value_type == slot_type)
    {
      return false;
    }
    let Some(block) = self.block_mut(block)
    else
    {
      return false;
    };
    block.instructions.push(Instruction::Store { slot,
                                                 value });
    true
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

  pub fn set_panic(&mut self, block: BlockId) -> bool
  {
    let Some(block) = self.block_mut(block)
    else
    {
      return false;
    };
    if block.terminator.is_some()
    {
      return false;
    }
    block.terminator = Some(Terminator::Panic);
    true
  }

  pub(super) fn block(&self, block: BlockId) -> Option<&Block>
  {
    self.blocks
        .get(block.0 as usize)
        .filter(|candidate| candidate.id == block)
  }

  pub(super) fn block_mut(&mut self, block: BlockId) -> Option<&mut Block>
  {
    self.blocks
        .get_mut(block.0 as usize)
        .filter(|candidate| candidate.id == block)
  }

  pub(super) fn value(&self, value: ValueId) -> Option<&Value>
  {
    self.values
        .get(value.0 as usize)
        .filter(|candidate| candidate.id == value)
  }

  fn slot(&self, slot: SlotId) -> Option<&Slot>
  {
    self.slots.get(slot.0 as usize).filter(|candidate| candidate.id == slot)
  }
}

#[derive(Clone, Copy)]
enum I32Op
{
  Add,
  Sub,
  Mul,
  Eq,
  Lt,
  Le,
  Gt,
  Ge,
}

fn i32_instruction(op: I32Op, result: ValueId, left: ValueId, right: ValueId) -> Instruction
{
  match op
  {
    I32Op::Add => Instruction::AddI32 { result,
                                        left,
                                        right },
    I32Op::Sub => Instruction::SubI32 { result,
                                        left,
                                        right },
    I32Op::Mul => Instruction::MulI32 { result,
                                        left,
                                        right },
    I32Op::Eq => Instruction::EqI32 { result,
                                      left,
                                      right },
    I32Op::Lt => Instruction::LtI32 { result,
                                      left,
                                      right },
    I32Op::Le => Instruction::LeI32 { result,
                                      left,
                                      right },
    I32Op::Gt => Instruction::GtI32 { result,
                                      left,
                                      right },
    I32Op::Ge => Instruction::GeI32 { result,
                                      left,
                                      right },
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

#[cfg(test)]
mod tests
{
  use super::super::{type_from_name, type_name};
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
  fn models_typed_stack_slot_load_and_store()
  {
    let mut function = Function::definition("Memory", Type::I32, vec![]);
    let entry = function.append_block("entry");
    let slot = function.add_slot(Type::I32).expect("slot should be added");
    let value = function.add_const_i32(entry, 7).expect("constant should be added");

    assert!(function.add_store(entry, slot, value));
    let loaded = function.add_load(entry, slot).expect("load should be added");
    assert!(function.set_return(entry, Some(loaded)));
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
  fn adds_i32_const_instruction()
  {
    let mut function = Function::definition("main", Type::I32, vec![]);
    let block = function.append_block("entry");
    let result = function.add_const_i32(block, 0).expect("i32 const should be added");

    assert_eq!(result, ValueId(0));
    assert!(function.set_return(block, Some(result)));
  }

  #[test]
  fn adds_i32_instruction_family()
  {
    let mut function = Function::definition("Compare", Type::BOOL, vec![]);
    let block = function.append_block("entry");
    let left = function.add_const_i32(block, 2).expect("left const should be added");
    let right = function.add_const_i32(block, 3).expect("right const should be added");
    assert_eq!(function.add_i32(block, left, right), Some(ValueId(2)));
    assert_eq!(function.sub_i32(block, left, right), Some(ValueId(3)));
    assert_eq!(function.mul_i32(block, left, right), Some(ValueId(4)));
    assert_eq!(function.eq_i32(block, left, right), Some(ValueId(5)));
    assert_eq!(function.lt_i32(block, left, right), Some(ValueId(6)));
    assert_eq!(function.le_i32(block, left, right), Some(ValueId(7)));
    assert_eq!(function.gt_i32(block, left, right), Some(ValueId(8)));
    let result = function.ge_i32(block, left, right);

    assert_eq!(result, Some(ValueId(9)));
    assert!(function.set_return(block, result));
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
  fn sets_panic_terminator()
  {
    let mut function = Function::definition("Panic", Type::VOID, vec![]);
    let entry = function.append_block("entry");

    assert!(function.set_panic(entry));
    assert_eq!(function.blocks[0].terminator, Some(Terminator::Panic));
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
    assert_eq!(type_name(Type::I32), "i32");
    assert_eq!(type_name(Type::I64), "i64");
  }

  #[test]
  fn parses_primitive_type_names()
  {
    assert_eq!(type_from_name("void"), Some(Type::VOID));
    assert_eq!(type_from_name("i64"), Some(Type::I64));
    assert_eq!(type_from_name("i32"), Some(Type::I32));
    assert_eq!(type_from_name("unknown"), None);
  }
}
