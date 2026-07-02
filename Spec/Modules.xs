// module system:

//{
module declaration is optional.

If a file will be imported by another file,
it must contain a module declaration.

Only one module declaration is allowed per file.

module name does not have to match the file name.

namespace is optional.
namespace must be declared under module.
namespace does not use braces.
namespace declarations are chained.

public namespace is file-scoped.
If public namespace is used, top-level declarations in that file are public.

Non-public members are not visible from other modules.

Duplicate imports are allowed.
Duplicate imports are treated as one import.
}//

// module declaration
module Math;

// namespace
module Math;

namespace Advanced;

// chained namespace
module Math;

namespace Advanced;
namespace Algebra;

// equivalent full path:
// Math.Advanced.Algebra


// public namespace
module Math;

public namespace Advanced;

fn Add(a: int, b: int) => int {
    return a + b;
}


// normal import
imports Math;

fn Main() {
    result: int = Math.Add(1, 2);
}


// multiple imports
imports Stdio, Math, Collections;


// import alias
imports Math as M;

fn Main() {
    result: int = M.Add(1, 2);
}


// wildcard import
from Math imports *;

fn Main() {
    result: int = Add(1, 2);
}


// selected import
from Math imports Add;

fn Main() {
    result: int = Add(1, 2);
}


// selected multiple import
from Math imports Add, Subtract;

fn Main() {
    a: int = Add(1, 2);
    b: int = Subtract(5, 3);
}


// selected import alias
from Math imports Add as Sum;

fn Main() {
    result: int = Sum(1, 2);
}


// fully qualified enum access
imports Math;

fn Main() {
    color: Math.Color = Math.Color.Red;
}


// fully qualified namespace enum access
imports Math;

fn Main() {
    color: Math.Advanced.Color = Math.Advanced.Color.Red;
}


// direct enum access after wildcard import
from Math imports *;

fn Main() {
    color: Color = Color.Red;
}


// direct enum access after selected import
from Math imports Color;

fn Main() {
    color: Color = Color.Red;
}


// duplicate imports
imports Math;
imports Math;

imports Math, Math;

from Math imports Add;
from Math imports Add;

// Duplicate imports are valid and treated as one import.


// import name collision
from Math imports Add;
from Utils imports Add;

fn Main() {
    M.Add(1, 2);
    U.Add(1, 2);
}

// If the same member name is imported from multiple modules,
// the names are accessed through automatic module aliases.
// Math -> M
// Utils -> U


// VALID
module Calculator;

fn Add(a: int, b: int) => int {
    return a + b;
}

// This module is imported as Calculator.


// VALID
imports Calculator;

fn Main() {
    result: int = Calculator.Add(1, 2);
}


// VALID
fn Main() {
}

// module is optional if the file is not imported.


// VALID
module Math;

namespace Advanced;
namespace Algebra;

fn Add() {
}


// INVALID
fn Add(a: int, b: int) => int {
    return a + b;
}

// This file cannot be imported because it has no module declaration.


// INVALID
module Math;
module Utils;

// Only one module declaration is allowed per file.


// INVALID
namespace Advanced;

module Math;

// namespace must be declared under module.


// INVALID
namespace Advanced {
}

// namespace does not use braces.


// INVALID
module Math;

fn InternalHelper() {
}

// from another file:
from Math imports InternalHelper;

// Non-public members are not visible from other modules.
