// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

fn countdown<T>(value: T) -> T {
  if (value == 0) {
    return 7;
  }
  return countdown::<T>(value - 1);
}

fn main() -> Long {
  return countdown::<Long>(3);
}
