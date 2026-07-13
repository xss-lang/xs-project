// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// X# type system baseline.

// Primitive type names are nominal language types.
// Even when two user-defined types have the same fields, type identity is
// based on the declared name.
// Str is an immutable UTF-16 static string reference, like Rust &'static str.
// The compiler selects UTF-16LE or UTF-16BE automatically for the target/runtime situation.
// Optional<Str> is an owned optional string value, like Rust Option<String>.

valueStr: Str = "text";
valueBool: Bool = true;

valueByte: Byte = 255;
valueSByte: SByte = -1;
valueChar: Char = 'A';

valueShort: Short = -32'000;
valueLong: Long = 2'000'000'000;
valueInt: Int = 9'000'000'000;
valueInteger: Integer = 170'000'000'000'000'000'000;

valueUShort: UShort = 65'535;
valueULong: ULong = 4'000'000'000;
valueUInt: UInt = 18'000'000'000;
valueUInteger: UInteger = 340'000'000'000'000'000'000;

valueSFloat: SFloat = 1.0;
valueFloat: Float = 1.0;

// Optional<T> is resolved as if the compiler had inserted
// `imports optional; using namespace std::optional;` and brought
// std::optional::Optional<T> into scope as Optional<T>.
// The value constructors are std::optional::None and std::optional::Some(...).
// Users may import Optional explicitly, but normal source files do not need to.
// Optional<T> is not lowered through the enum data mechanism.
// There is no nullable T? type operator.
// Optional<Str> owns the optional string payload instead of being an optional
// borrowed/static Str reference.

name: Optional<Str> = None;
name = Some("Leitwolf");

// Result is also special. `imports result;` is optional; the compiler behaves
// as if `using namespace std::result;` existed for Result<T, E>, Ok(...), and
// Error(...). Most other std::* modules still require qualified names or
// explicit using declarations.

status: Result<Int, Error> = Ok(0);

emptyCanonical: std::optional::Optional<Str> = std::optional::None;
canonicalName: std::optional::Optional<Str> = std::optional::Some("Leitwolf");
shortName: Optional<Str> = canonicalName;

display: Str = name ?? "guest";
name ??= std::optional::Some("guest");

// Automatic unboxing from Optional<T> to T may fail. New code models that as
// Error rather than legacy exceptions.

unboxedName: Str = name;
forcedName: Str = name!;

user: Optional<User> = None;
city: Optional<Str> = user?.Address?.City;

fn NormalizeOptionalName(value: Optional<Str>) -> Result<Str, Error> {
    if (value == None) {
        return Error(Error {
            message: "name is missing",
        });
    }

    return Ok(value!);
}
