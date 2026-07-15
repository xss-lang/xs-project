/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl Function
{
  pub fn add_array_get(&mut self, block: BlockId, array: ValueId, index: ValueId, element_type: Type)
                       -> Option<ValueId>
  {
    if self.value(array)?.value_type.kind != TypeKind::Array ||
       self.value(index)?.value_type != Type::I64 ||
       element_type == Type::VOID
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: element_type });
    self.block_mut(block)?.instructions.push(Instruction::ArrayGet { result,
                                                                     array,
                                                                     index });
    Some(result)
  }

  pub fn add_array_set(&mut self, block: BlockId, array: ValueId, index: ValueId, value: ValueId) -> Option<ValueId>
  {
    let array_type = self.value(array)?.value_type;
    if array_type.kind != TypeKind::Array || self.value(index)?.value_type != Type::I64 || self.value(value).is_none()
    {
      return None;
    }
    let result = ValueId(self.values.len() as u32);
    self.values.push(Value { id: result,
                             value_type: array_type });
    self.block_mut(block)?.instructions.push(Instruction::ArraySet { result,
                                                                     array,
                                                                     index,
                                                                     value });
    Some(result)
  }
}
