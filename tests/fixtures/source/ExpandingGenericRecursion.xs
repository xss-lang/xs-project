// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn grow<T>(value: T) -> T {
  return grow::<[T]>([value]);
}

fn main() -> Long {
  return grow::<Long>(7);
}
