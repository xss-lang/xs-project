/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{DiagnosticCode, Parser};
use crate::xlil::{Module, Type};

impl Parser<'_>
{
  pub(super) fn array_type(&mut self, module: &mut Module) -> bool
  {
    let line = self.line_number();
    let Some(record) = self.next_line()
    else
    {
      return self.invalid_array(line, "XLIL array type record is invalid");
    };
    let Some(record) = record.strip_prefix(".array %a")
    else
    {
      return self.invalid_array(line, "XLIL array type record is invalid");
    };
    let Some((id, definition)) = record.split_once(" : ")
    else
    {
      return self.invalid_array(line, "XLIL array type id is missing");
    };
    let Ok(id) = id.parse::<u32>()
    else
    {
      return self.invalid_array(line, "XLIL array type id is invalid");
    };
    if id != self.array_type_count
    {
      return self.invalid_array(line, "XLIL array type ids must be sequential");
    }
    let Some((element, length)) = definition.rsplit_once(" x ")
    else
    {
      return self.invalid_array(line, "XLIL array type definition is invalid");
    };
    let Some(element) = self.type_name(element, line)
    else
    {
      return false;
    };
    let Ok(length) = length.parse::<u64>()
    else
    {
      return self.invalid_array(line, "XLIL array length is invalid");
    };
    if module.add_array_type(element, length) != Some(Type::array(id))
    {
      return self.invalid_array(line, "XLIL array type is invalid or duplicated");
    }
    self.array_type_count += 1;
    true
  }

  fn invalid_array(&mut self, line: usize, message: &str) -> bool
  {
    self.report(DiagnosticCode::InvalidType, line, message);
    false
  }
}
