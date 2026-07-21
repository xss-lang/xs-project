// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

fn is_even(value: Long) -> Bool {
  if (value == 0) {
    return true;
  }
  return is_odd(value - 1);
}
