// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// generics system:

//
// Generics are supported for classes and functions.
//
// Generics use monomorphization.
//
// Each concrete type produces a separate specialization.
//
// Type erasure does not exist.
//
// Generics are zero-cost abstractions.
//
// Generics must not introduce hidden allocations.
//
// Generic parameters may be constrained by interfaces.
//
// Multiple generic parameters are supported.
//
// Multiple constraints are supported.
//
// Default generic parameters are not supported.
//


// generic class

class Box<T> {
    value: T;
}


// generic class usage

numberBox: Box<Int>;

textBox: Box<Str>;


// generic class with multiple parameters

class Pair<T, U> {
    first: T;
    second: U;
}


pair: Pair<Str, Int>;


// generic function

fn Print<T>(value: T) {
}


// generic function usage

Print<Int>(42);

Print<Str>("Hello");


// generic class constraint

interface Printable {
    fn Print();
}

class Box<T: Printable> {
    value: T;
}


// generic function constraint

interface Runnable {
    fn Run();
}

fn Execute<T: Runnable>(value: T) {
    value.Run();
}


// multiple constraints

interface Runnable {
    fn Run();
}

interface Printable {
    fn Print();
}

fn Execute<T: Runnable, Printable>(value: T) {

    value.Run();

    value.Print();
}


// multiple generic parameters with constraints

class Storage<T: Printable, U: Runnable> {

    value: T;

    worker: U;
}


// monomorphization

class Box<T> {
    value: T;
}

intBox: Box<Int>;

strBox: Box<Str>;


// Compiler generates separate specializations.
//
// Example:
//
// Box_int
// Box_str


// generic collections

users: Collections.vector<Str>;

scores: Collections.hashmap<Str, Int>;


// generic enum data

enum data Result<T> {
    Ok: T,
    Error: Str,
}


// generic return type

fn CreateBox<T>(value: T) => Box<T> {

    box: Box<T>;

    box.value = value;

    return box;
}


// VALID

class Box<T> {
    value: T;
}


// VALID

class Pair<T, U> {
    first: T;
    second: U;
}


// VALID

fn Print<T>(value: T) {
}


// VALID

fn Execute<T: Runnable>(value: T) {
}


// VALID

fn Execute<T: Runnable, Printable>(value: T) {
}


// VALID

class Storage<T: Printable> {
    value: T;
}


// INVALID

class Box<T = Str> {
}


// Default generic parameters are not supported.


// INVALID

fn Print<T = Str>(value: T) {
}


// Default generic parameters are not supported.


// INVALID

class Storage<T: UnknownInterface> {
    value: T;
}


// Constraint type must exist.


// performance guarantees

//
// Generics use monomorphization.
//
// No type erasure.
//
// No hidden allocation.
//
// Zero-cost abstraction.
//
// Allocation behavior must remain visible
// or predictable.
//
