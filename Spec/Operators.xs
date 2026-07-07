// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// operators system:

//{
Comparison Operators:
==
!=
>
<
>=
<=

Logical Operators:
&&
||
!

Bitwise Operators:
&
|
}//

fn Main() {
    a: int = 10;
    b: int = 20;

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
    result: int = a & b;
    result = a | b;
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
value: int = 10 & 20;


// VALID
value: int = 10 | 20;
