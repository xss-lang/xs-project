// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// enum system:

//{
enum:
- May be empty.
- Cannot contain typed variants.
- Has no numeric value.
- Is not implicitly convertible to integers.

enum data:
- Must contain at least one typed variant.
- May contain both typed and non-typed variants.
- Each variant may contain at most one value.
- Payload type may be any valid type.

Visibility modifiers:
- public
- private
- protected
- internal

Trailing commas are supported but not required.
}//

// basic enum
enum Color {
    Red,
    Green,
    Blue,
}

// empty enum
enum Empty {
}

// enum usage
color: Color = Color.Red;


// data enum
enum data Token {
    Identifier: Str,
    Integer: Int,
    Plus,
}

// data enum usage
token: Token = Token.Identifier("hello");
number: Token = Token.Integer(42);
plus: Token = Token.Plus;


// payload type examples
class User {
}

enum data Value {
    Text: Str,
    Number: Int,
    User: User,
    Color: Color,
}


// visibility
public enum Color {
    Red,
    Green,
    Blue,
}

internal enum Color {
    Red,
    Green,
    Blue,
}

public enum data Token {
    Identifier: Str,
    Integer: Int,
    Plus,
}

internal enum data Token {
    Identifier: Str,
    Integer: Int,
    Plus,
}


// trailing comma
enum Color {
    Red,
    Green,
    Blue,
}

enum Color {
    Red,
    Green,
    Blue
}


// pattern matching
fn Parse(token: Token) {
    match (token) {
        Token.Identifier(value) -> {
        },
        Token.Integer(value) -> {
        },
        Token.Plus -> {
        },
        else -> {
        },
    };
}


// VALID
enum Color {
    Red,
    Green,
    Blue,
}


// VALID
enum Empty {
}


// VALID
enum data Token {
    Identifier: Str,
}


// VALID
enum data Token {
    Identifier: Str,
    Integer: Int,
    Plus,
    Minus,
}


// VALID
token: Token = Token.Identifier("hello");


// VALID
token: Token = Token.Integer(42);


// VALID
token: Token = Token.Plus;


// INVALID
enum Color {
    Red: Int,
}
// enum variants cannot contain types.


// INVALID
enum data Empty {
}
// enum data requires at least one typed variant.


// INVALID
enum data Token {
    Plus,
    Minus,
}
// enum data requires at least one typed variant.


// INVALID
enum data Token {
    Position: Int, Int,
}
// enum data variants can contain at most one value.


// INVALID
enum data Token {
    Position: (Int, Int),
}
// tuple payloads are not supported.


// INVALID
value: Int = Color.Red;
// enums cannot be converted to integers.


// INVALID
color: Color = 0;
// integers cannot be converted to enums.
