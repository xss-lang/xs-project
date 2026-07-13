/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

pub fn format_units(units: &[u16]) -> String
{
  format_with_prefix("utf16", units)
}

pub fn format_encoded(encoding: crate::xlil::Utf16Encoding, units: &[u16]) -> String
{
  format_with_prefix(encoding.text_name(), units)
}

fn format_with_prefix(prefix: &str, units: &[u16]) -> String
{
  let mut output = format!("{prefix} [");
  for (index, unit) in units.iter().enumerate()
  {
    if index != 0
    {
      output.push_str(", ");
    }
    let _ = write!(output, "0x{unit:04x}");
  }
  output.push(']');
  output
}

pub fn parse_units(value: &str) -> Option<Vec<u16>>
{
  parse_with_prefix(value, "utf16")
}

pub fn parse_encoded(value: &str) -> Option<(crate::xlil::Utf16Encoding, Vec<u16>)>
{
  for encoding in [crate::xlil::Utf16Encoding::LittleEndian,
                   crate::xlil::Utf16Encoding::BigEndian]
  {
    if let Some(units) = parse_with_prefix(value, encoding.text_name())
    {
      return Some((encoding, units));
    }
  }
  None
}

fn parse_with_prefix(value: &str, prefix: &str) -> Option<Vec<u16>>
{
  let values = value.strip_prefix(&format!("{prefix} ["))?.strip_suffix(']')?;
  let units = if values.is_empty()
  {
    Vec::new()
  }
  else
  {
    values.split(", ")
          .map(|value| {
            let hexadecimal = value.strip_prefix("0x")?;
            (hexadecimal.len() == 4).then_some(())?;
            u16::from_str_radix(hexadecimal, 16).ok()
          })
          .collect::<Option<Vec<_>>>()?
  };
  String::from_utf16(&units).ok()?;
  Some(units)
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn roundtrips_utf16_code_units()
  {
    let units = "Leitwolf 🐺".encode_utf16().collect::<Vec<_>>();
    assert_eq!(parse_units(&format_units(&units)), Some(units));
    assert_eq!(parse_units("utf16 [0xd800]"), None);
    assert_eq!(parse_units("utf16 [0x41]"), None);
    assert_eq!(parse_encoded("utf16le [0x0041]"),
               Some((crate::xlil::Utf16Encoding::LittleEndian, vec![0x41])));
  }
}
