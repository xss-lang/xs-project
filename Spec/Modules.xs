// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// module system:

//
// module declaration is optional.
//
// If a file will be imported by another file,
// it must contain a module declaration.
//
// Only one module declaration is allowed per file.
//
// module name does not have to match the file name.
//
// namespace is optional.
// namespace must be declared under module.
// namespace does not use braces.
// namespace declarations are chained.
//
// public namespace is file-scoped.
// If public namespace is used, top-level declarations in that file are public.
//
// Non-public members are not visible from other modules.
//
// Duplicate imports are allowed.
// Duplicate imports are treated as one import.
//

// module declaration
module math;

// namespace
module math;

namespace advanced;

// chained namespace
module math;

namespace advanced;
namespace algebra;

// equivalent full path:
// math::advanced::algebra


// public namespace
module math;

public namespace advanced;

fn add(a: Int, b: Int) -> Int {
    return a + b;
}


// normal import
// imports makes the module usable through its qualified name.
// It does not place the module's public symbols directly in local scope.
imports math;

fn main() {
    result: Int = math::add(1, 2);
}


// multiple imports
imports stdio, math, collections;


// namespace using declaration
using namespace math;

fn main() {
    result: Int = add(1, 2);
}


// using declaration
// using declarations place one selected public name in local scope.
using math::add;

fn main() {
    result: Int = add(1, 2);
}


// multiple using declarations
using math::add;
using math::subtract;

fn main() {
    a: Int = add(1, 2);
    b: Int = subtract(5, 3);
}


// using alias declaration
using sum = math::add;

fn main() {
    result: Int = sum(1, 2);
}


// fully qualified enum access
imports math;

fn main() {
    color: math::Color = math::Color::Red;
}


// fully qualified namespace enum access
imports math;

fn main() {
    color: math::advanced::Color = math::advanced::Color::Red;
}


// direct enum access after namespace using
using namespace math;

fn main() {
    color: Color = Color::Red;
}


// direct enum access after using declaration
using math::Color;

fn main() {
    color: Color = Color::Red;
}


// duplicate imports
imports math;
imports math;

imports math, math;

using math::add;
using math::add;

// Duplicate imports are valid and treated as one import.


// using name collision
using math::add;
using utils::add;

fn main() {
    math::add(1, 2);
    utils::add(1, 2);
}

// If the same local name is opened from multiple modules,
// it is a diagnostic. Keep the calls qualified instead.


// VALID
module calculator;

fn add(a: Int, b: Int) -> Int {
    return a + b;
}

// This module is imported as calculator.


// VALID
imports calculator;

fn main() {
    result: Int = calculator::add(1, 2);
}


// VALID
fn main() {
}

// module is optional if the file is not imported.


// VALID
module math;

namespace advanced;
namespace algebra;

fn add() {
}


// INVALID
fn add(a: Int, b: Int) -> Int {
    return a + b;
}

// This file cannot be imported because it has no module declaration.


// INVALID
module math;
module utils;

// Only one module declaration is allowed per file.


// INVALID
namespace advanced;

module math;

// namespace must be declared under module.


// INVALID
namespace advanced {
}

// namespace does not use braces.


// INVALID
module math;

fn internal_helper() {
}

// from another file:
using math::internal_helper;

// Non-public members are not visible from other modules.
