// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long {
  value: Long = 0;
  while (true) {
    value += 1;
    if (value == 3) {
      continue;
    }
    if (value == 5) {
      break;
    }
  }
  return value + 2;
}
