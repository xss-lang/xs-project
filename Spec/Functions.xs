// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// function and file structure:

//{
Semicolons are required.
Top-level execution is not supported.
Top-level functions are allowed.
}//

fn Add(a: Int, b: Int) => Int {
  return a + b;
}

// => declares the return type.
// If => is omitted, the function returns void.
// There is no separate void keyword.

class Program {
  fn Test() {
  }
}

// INVALID: top-level execution is not supported.
fn Add(a: Int, b: Int) => Int {
  return a + b;
}

Add(1, 2);

// VALID: top-level functions are allowed.
fn Main() {
}

// VALID: omitted => means void.
fn Test() {
}

// INVALID: => void is not valid.
fn Test() => void {
}
