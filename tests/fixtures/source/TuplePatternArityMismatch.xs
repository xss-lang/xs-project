// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long
{
  values: [(Long, Long); 1] = [(1, 2)];
  for ((left, right, extra) in values)
  {
    return left + right + extra;
  }
  return 0;
}
