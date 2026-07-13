// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long {
  selected: Bool = false;
  match (selected) {
    true -> {
      return 1;
    },
    false -> {
      return 7;
    },
    else -> {
      return 3;
    },
  }
  return 0;
}
