// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long
{
  val person: (score: Long, active: Bool) = (score: 2, active: true);
  person.score = 7;
  person.active = false;
  if (!person.active)
  {
    return person.score;
  }
  return 0;
}
