// fonksiyon ve dosya yapısı:

//{
Semicolon zorunludur.
Top-level execution yoktur.
Top-level fn izinlidir.
}//

fn Add(a: int, b: int) => int {
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
fn Add(a: int, b: int) => int {
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
