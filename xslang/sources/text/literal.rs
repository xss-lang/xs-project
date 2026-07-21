/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

pub fn decode_character(token: &str) -> Option<u16>
{
  let inner = token.strip_prefix('\'')?.strip_suffix('\'')?;
  if let Some(escape) = inner.strip_prefix('\\')
  {
    return decode_escape(escape);
  }
  let mut characters = inner.chars();
  let value = characters.next()?;
  if characters.next().is_some()
  {
    return None;
  }
  u16::try_from(u32::from(value)).ok()
}

pub fn format_character(value: u16) -> String
{
  match value
  {
    0 => "'\\0'".to_string(),
    7 => "'\\a'".to_string(),
    8 => "'\\b'".to_string(),
    9 => "'\\t'".to_string(),
    10 => "'\\n'".to_string(),
    11 => "'\\v'".to_string(),
    12 => "'\\f'".to_string(),
    13 => "'\\r'".to_string(),
    value if value == u16::from(b'\'') => "'\\\''".to_string(),
    value if value == u16::from(b'\\') => "'\\\\'".to_string(),
    0x20..=0x7e => format!("'{}'", char::from_u32(u32::from(value)).unwrap_or('?')),
    _ => format!("'\\u{value:04x}'"),
  }
}

fn decode_escape(value: &str) -> Option<u16>
{
  let simple = match value
  {
    "'" => Some(u16::from(b'\'')),
    "\"" => Some(u16::from(b'\"')),
    "\\" => Some(u16::from(b'\\')),
    "0" => Some(0),
    "a" => Some(7),
    "b" => Some(8),
    "f" => Some(12),
    "n" => Some(10),
    "r" => Some(13),
    "t" => Some(9),
    "v" => Some(11),
    _ => None,
  };
  simple.or_else(|| decode_hex_escape(value))
}

fn decode_hex_escape(value: &str) -> Option<u16>
{
  let (digits, width) = if let Some(digits) = value.strip_prefix('x')
  {
    (digits, 2)
  }
  else if let Some(digits) = value.strip_prefix('u')
  {
    (digits, 4)
  }
  else
  {
    (value.strip_prefix('U')?, 8)
  };
  (digits.len() == width).then_some(())?;
  let value = u32::from_str_radix(digits, 16).ok()?;
  u16::try_from(value).ok()
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn decodes_ascii_and_utf16_character_tokens()
  {
    assert_eq!(decode_character("'A'"), Some(0x0041));
    assert_eq!(decode_character("'Ω'"), Some(0x03a9));
    assert_eq!(decode_character("'😀'"), None);
    assert_eq!(decode_character("'\\n'"), Some(0x000a));
    assert_eq!(decode_character("'\\u03a9'"), Some(0x03a9));
    assert_eq!(decode_character("'\\U0000ffff'"), Some(0xffff));
    assert_eq!(decode_character("'\\U00010000'"), None);
    assert_eq!(format_character(0x0041), "'A'");
    assert_eq!(format_character(0x03a9), "'\\u03a9'");
  }
}
