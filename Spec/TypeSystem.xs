// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// X# type system baseline.

// Primitive type names are nominal language types.
// Even when two user-defined types have the same fields, type identity is
// based on the declared name.
// Str is an immutable UTF-16 static string reference, like Rust &'static str.
// The compiler selects UTF-16LE or UTF-16BE automatically for the target/runtime situation.
// Optional<Str> is an owned optional string value, like Rust Option<String>.

value_str: Str = "text";
value_bool: Bool = true;

value_byte: Byte = 255;
value_s_byte: SByte = -1;
value_char: Char = 'A';

value_short: Short = -32'000;
value_long: Long = 2'000'000'000;
value_int: Int = 9'000'000'000;
value_integer: Integer = 170'000'000'000'000'000'000;

value_u_short: UShort = 65'535;
value_u_long: ULong = 4'000'000'000;
value_u_int: UInt = 18'000'000'000;
value_u_integer: UInteger = 340'000'000'000'000'000'000;

value_s_float: SFloat = 1.0;
value_float: Float = 1.0;

// Optional<T> is resolved as if the compiler had inserted
// `imports optional; using namespace std::optional;` and brought
// std::optional::Optional<T> into scope as Optional<T>.
// The compiler makes the None and Some(...) value constructors available.
// Users may import Optional explicitly, but normal source files do not need to.
// Optional<T> is not lowered through the enum data mechanism.
// There is no nullable T? type operator.
// Optional<Str> owns the optional string payload instead of being an optional
// borrowed/static Str reference.

name: Optional<Str> = None;
name = Some("Leitwolf");

// Result is also special. `imports result;` is optional; the compiler behaves
// as if `using namespace std::result;` existed for Result<T>, Result<T, E>, Ok(...), and
// Error(...). Most other std::* modules still require qualified names or
// explicit using declarations.

status: Result<Int, Error> = Ok(0);

empty_canonical: Optional<Str> = None;
canonical_name: Optional<Str> = Some("Leitwolf");
short_name: Optional<Str> = canonical_name;

display: Str = name ?? "guest";
name ??= Some("guest");

// Automatic unboxing from Optional<T> to T may fail. New code models that as
// Error values through Result.

unboxed_name: Str = name;
forced_name: Str = name!;

user: Optional<User> = None;
city: Optional<Str> = user?.Address?.City;

fn normalize_optional_name(value: Optional<Str>) -> Result<Str, Error> {
    if (value == None) {
        return Error(Error {
            message: "name is missing",
        });
    }

    return Ok(value!);
}
