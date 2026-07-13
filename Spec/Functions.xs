// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// function and file structure:

//
// Semicolons are required.
// Top-level execution is not supported.
// Top-level functions are allowed.
//

imports stdio, result;

fn Add(a: Int, b: Int) -> Int {
  return a + b;
}

// -> declares the return type.
// If -> is omitted, the function returns void.
// There is no separate void keyword.

class Program {
  fn Test() {
  }
}

// INVALID EXAMPLE:
// Top-level execution is not supported. This snippet is intentionally invalid
// and is not part of the valid declarations above.
fn Add(a: Int, b: Int) -> Int {
  return a + b;
}

Add(1, 2);

// VALID: top-level functions are allowed.
fn Main() {
}

// VALID: omitted -> means void.
fn Test() {
}

// INVALID EXAMPLE:
// `-> void` is not valid. Omit `->` for a void-returning function.
fn Test() -> void {
}


// named and inferred helper examples

fn Clamp(value: Int, minimum: Int, maximum: Int) -> Int {
  if (value < minimum) {
    return minimum;
  }

  if (value > maximum) {
    return maximum;
  }

  return value;
}

fn DescribeUser(name: Str, age: Int) -> Str {
  return format!("{} ({})", name, age);
}

fn TryParseId(text: Str) -> Result<Int, Error> {
  parsed: Int = Int.Parse(text);
  return Ok(parsed);
}
