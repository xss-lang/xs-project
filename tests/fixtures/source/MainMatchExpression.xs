// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

fn main() -> Long {
  value: Long = 2;
  result: Long = match (value) {
    0 -> { 1 },
    2 -> { 7 },
    else -> { 3 },
  };
  return result;
}
