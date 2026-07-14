// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn factorial(value: Long) -> Long {
  if (value <= 1) {
    return 1;
  }
  return value * factorial(value - 1);
}

fn main() -> Long {
  return factorial(5);
}
