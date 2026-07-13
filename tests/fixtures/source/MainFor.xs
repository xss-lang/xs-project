// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long {
  sum: Long = 0;
  for (i: Long = 0; i < 5; i += 1) {
    if (i == 2) {
      continue;
    }
    sum += i;
  }
  return sum;
}
