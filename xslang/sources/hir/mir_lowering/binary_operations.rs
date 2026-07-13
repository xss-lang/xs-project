/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::type_check::BinaryOperator;
use crate::xlil::{I32BinaryOperation, I64BinaryOperation, I64ComparisonOperation};

pub(super) const fn binary_i32_operation(operator: BinaryOperator) -> Option<I32BinaryOperation>
{
  Some(match operator
  {
    BinaryOperator::Div => I32BinaryOperation::Div,
    BinaryOperator::Rem => I32BinaryOperation::Rem,
    BinaryOperator::BitAnd => I32BinaryOperation::BitAnd,
    BinaryOperator::BitOr => I32BinaryOperation::BitOr,
    BinaryOperator::BitXor => I32BinaryOperation::BitXor,
    BinaryOperator::ShiftLeft => I32BinaryOperation::ShiftLeft,
    BinaryOperator::ShiftRight => I32BinaryOperation::ShiftRight,
    _ => return None,
  })
}

pub(super) const fn binary_i64_operation(operator: BinaryOperator) -> Option<I64BinaryOperation>
{
  Some(match operator
  {
    BinaryOperator::Div => I64BinaryOperation::Div,
    BinaryOperator::Rem => I64BinaryOperation::Rem,
    BinaryOperator::BitAnd => I64BinaryOperation::BitAnd,
    BinaryOperator::BitOr => I64BinaryOperation::BitOr,
    BinaryOperator::BitXor => I64BinaryOperation::BitXor,
    BinaryOperator::ShiftLeft => I64BinaryOperation::ShiftLeft,
    BinaryOperator::ShiftRight => I64BinaryOperation::ShiftRight,
    _ => return None,
  })
}

pub(super) const fn comparison_i64_operation(operator: BinaryOperator) -> Option<I64ComparisonOperation>
{
  Some(match operator
  {
    BinaryOperator::Less => I64ComparisonOperation::Less,
    BinaryOperator::LessEqual => I64ComparisonOperation::LessEqual,
    BinaryOperator::Greater => I64ComparisonOperation::Greater,
    BinaryOperator::GreaterEqual => I64ComparisonOperation::GreaterEqual,
    _ => return None,
  })
}
