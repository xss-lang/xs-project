// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// fonksiyon ve dosya yapısı:

//{
Semicolon zorunludur.
Top-level execution yoktur.
Top-level fn izinlidir.
}//

fn Add(a: Int, b: Int) => Int {
  return a + b;
}

// => return type belirtir.
// => yazılmaz ise fonksiyon void kabul edilir.
// void diye ayrı bir keyword yoktur.

class Program {
  fn Test() {
  }
}

// INVALID: top-level execution yoktur.
fn Add(a: Int, b: Int) => Int {
  return a + b;
}

Add(1, 2);

// VALID: top-level fn izinlidir.
fn Main() {
}

// VALID: => yoksa void kabul edilir.
fn Test() {
}

// INVALID: => void yoktur.
fn Test() => void {
}
