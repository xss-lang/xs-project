// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn wide_ops(left: Int, right: Int) -> Int {
  return (left / right) & (left % right) & (left | right) & (left ^ right) & ((left << right) >> right);
}

fn wide_compare(left: Int, right: Int) -> Bool {
  return left >= right;
}

fn main() -> Long {
  return 7;
}
