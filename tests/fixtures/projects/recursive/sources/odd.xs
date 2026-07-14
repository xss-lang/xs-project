// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn is_odd(value: Long) -> Bool {
  if (value == 0) {
    return false;
  }
  return is_even(value - 1);
}
