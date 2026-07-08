// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// control flow system:

//{
match is not an expression.
match is a statement.

Supported statements:
- if
- else if
- else
- for
- while
- break
- continue
- match
}//

fn Main() {
    x: Int = 5;

    if (x > 0) {
        x = 10;
    }

    if (x > 0) {
        x = 10;
    }
    else {
        x = 0;
    }

    if (x > 0) {
        x = 10;
    }
    else if (x < 0) {
        x = -10;
    }
    else {
        x = 0;
    }

    for (i: Int = 0; i < 10; i++) {
        if (i == 5) {
            break;
        }
    }

    for (i: Int = 0; i < 10; i++) {
        if (i == 5) {
            continue;
        }
    }

    while (x < 10) {
        x = x + 1;
    }

    match (x) {
        0 -> {
            x = 1;
        },
        1 -> {
            x = 2;
        },
        else -> {
            x = 0;
        },
    }
}


// VALID
fn TestIf(x: Int) {
    if (x > 0) {
        x = 1;
    }
}


// VALID
fn TestElseIf(x: Int) {
    if (x > 0) {
        x = 1;
    }
    else if (x < 0) {
        x = -1;
    }
    else {
        x = 0;
    }
}


// VALID
fn TestFor() {
    for (i: Int = 0; i < 10; i++) {
        continue;
    }
}


// VALID
fn TestWhile() {
    x: Int = 0;

    while (x < 10) {
        x = x + 1;
    }
}


// VALID
fn TestMatch(x: Int) {
    match (x) {
        0 -> {
            x = 10;
        },
        1 -> {
            x = 20;
        },
        else -> {
            x = 0;
        },
    }
}


// INVALID
fn InvalidMatchExpression(x: Int) {
    y: Int = match (x) {
        0 -> {
            10;
        },
        else -> {
            0;
        },
    };
}
// match is a statement, not an expression.


// INVALID
fn InvalidBreak() {
    break;
}
// break can only be used inside loops.


// INVALID
fn InvalidContinue() {
    continue;
}
// continue can only be used inside loops.
