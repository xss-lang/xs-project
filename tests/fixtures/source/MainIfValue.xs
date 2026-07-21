// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

fn identity(value: Long) -> Long {
  return value;
}

fn main() -> Long {
  selected: Long = if (true) { 7 } else { 3 };
  return identity(if (selected == 7) { selected } else { 9 });
}
