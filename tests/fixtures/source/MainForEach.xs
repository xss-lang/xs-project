// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long
{
  values: [Long; 5] = [1, 2, 3, 4, 5];
  total: Long = 0;
  for (value in values)
  {
    if (value == 2)
    {
      continue;
    }
    if (value == 5)
    {
      break;
    }
    total += value;
  }
  return total;
}
