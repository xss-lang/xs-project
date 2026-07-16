// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long
{
  values: [Long; 2] = [1, 2];
  for (item: Bool in values)
  {
    if (item)
    {
      return 1;
    }
  }
  return 0;
}
