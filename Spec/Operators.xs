// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// operators system:

//
// Comparison Operators:
// ==
// !=
// >
// <
// >=
// <=
//
// Logical Operators:
// &&
// ||
// !
//
// Bitwise Operators:
// &
// |
//
// Optional Operators:
// ?.
// ??
// ??=
// !
//

fn Main() {
    a: Int = 10;
    b: Int = 20;

    // equality
    if (a == b) {
    }

    if (a != b) {
    }

    // comparison
    if (a > b) {
    }

    if (a < b) {
    }

    if (a >= b) {
    }

    if (a <= b) {
    }

    // logical
    if (a > 0 && b > 0) {
    }

    if (a > 0 || b > 0) {
    }

    if (!(a == b)) {
    }

    // bitwise
    result: Int = a & b;
    result = a | b;

    // optional conditional access
    maybeUser: Optional<User> = None;
    maybeName: Optional<Str> = maybeUser?.Name;

    // optional coalescing
    displayName: Str = maybeName ?? "guest";

    // optional coalescing assignment
    maybeName ??= STD.Optional.Some("guest");

    // optional-forgiving unboxing
    forcedName: Str = maybeName!;
}


// VALID
if (1 == 1) {
}


// VALID
if (1 != 2) {
}


// VALID
if (true && true) {
}


// VALID
if (true || false) {
}


// VALID
if (!false) {
}


// VALID
value: Int = 10 & 20;


// VALID
value: Int = 10 | 20;


// VALID
name: Optional<Str> = None;
display: Str = name ?? "fallback";


// VALID
name ??= STD.Optional.Some("fallback");


// VALID
forced: Str = name!;
