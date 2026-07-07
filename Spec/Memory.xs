// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// memory system:

//{
Memory management is based on:

- Ownership
- Move Semantics
- Borrow Checking
- Lifetime Analysis
- RAII
- Arena Allocation

Garbage Collection does not exist.

All values are move-by-default.

Assignments move ownership.
Function arguments move ownership.
Return values move ownership.

After a move, the source value becomes invalid.

Ownership is verified at compile time.

Borrow checking is verified at compile time.

Lifetime analysis is verified at compile time.

Allocation must be visible or predictable.

Hidden allocations are forbidden.

Zero-cost abstractions are required.

Iterators, generics and closures must not introduce
runtime allocations unless explicitly documented.

Small short-lived values should remain on the stack.

Escape analysis may move values when necessary.

Resource destruction is deterministic.

Destructors execute in LIFO order.

nil is a valid value.
}//

// ownership

user: User = new();

user2: User = user;

// user moved
// user invalid


// move by function call

fn Consume(user: User) {
}

fn Main() {
    user: User = new();

    Consume(user);

    // user invalid
}


// move by return

fn CreateUser() => User {
    user: User = new();

    return user;
}

fn Main() {
    user: User = CreateUser();
}


// immutable borrow

user: User = new();

a: &User = &user;
b: &User = &user;


// mutable borrow

user: User = new();

a: &mut User = &mut user;


// borrow rules

//{
At any given time, a value may have:

- Exactly one mutable borrow (&mut T)

or

- One or more immutable borrows (&T)

Mutable and immutable borrows cannot coexist.
}//


// VALID

user: User = new();

a: &User = &user;
b: &User = &user;


// VALID

user: User = new();

a: &mut User = &mut user;


// INVALID

user: User = new();

a: &mut User = &mut user;
b: &mut User = &mut user;


// INVALID

user: User = new();

a: &User = &user;
b: &mut User = &mut user;


// INVALID

user: User = new();

a: &mut User = &mut user;

user.GetName();


// lifetime example

fn Print(user: &User) {
}

fn Main() {
    user: User = new();

    Print(&user);
}


// INVALID

fn GetUser() => &User {
    user: User = new();

    return &user;
}

// reference escapes its lifetime


// destructors

class File {
    File.Drop() {
        std.cout << "File closed\n";
    }
}


// destructor execution

fn Main() {
    file: File = new();

} // File.Drop() runs here


// moved values

file: File = new();

file2: File = file;

// file invalid
// only file2 owns the resource


// LIFO destruction order

fn Main() {
    a: File = new();
    b: File = new();
    c: File = new();
}

// destruction order:
//
// c.Drop()
// b.Drop()
// a.Drop()


// nil values

name: str = nil;

enabled: bool = nil;

user: User = nil;


// VALID

if (name == nil) {
}


// VALID

if (enabled == nil) {
}


// INVALID

if (enabled) {
}

// condition cannot be nil


// stack allocation

point: Point = Point(10, 20);

// compiler may keep this value on the stack


// escape analysis

fn CreatePoint() => Point {
    point: Point = Point(10, 20);

    return point;
}

// compiler performs escape analysis


// zero-cost abstractions

iter: Iterator<int> = numbers.Iterator();


// no hidden allocation

// VALID
// allocation is explicit

users: Collections.vector<User> =
    Collections.vector.new();


// INVALID DESIGN

result: str = a + b;

// if allocation occurs here,
// it must be visible or warned by the compiler


// performance principles

//{
No Garbage Collection.

No Hidden Allocation.

Deterministic Destruction.

Predictable Resource Release.

Ownership Verified At Compile Time.

Borrow Checking Verified At Compile Time.

Lifetime Analysis Verified At Compile Time.

Zero-Cost Abstractions.

Allocation Must Be Visible Or Predictable.
}//
