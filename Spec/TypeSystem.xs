// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// X# type system baseline.

// Primitive type names are nominal language types.
// Even when two user-defined types have the same fields, type identity is
// based on the declared name.
// Str is an immutable UTF-16 static-string-reference counterpart.

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

// Optional<T> is resolved as if the compiler had inserted `imports Optional`
// and brought STD.Optional.Optional<T> into scope as Optional<T>.
// The value constructors are STD.Optional.None and STD.Optional.Some(...).
// Users may import Optional explicitly, but normal source files do not need to.
// Optional<T> is not lowered through the enum data mechanism.
// There is no nullable T? type operator.

name: Optional<Str> = None;
name = Some("Leitwolf");

// Result is also special. The compiler behaves as if `imports Result;` existed
// and brings the short Result<T, E> wrapper name into scope. Most other STD
// modules still require qualified STD.* names or explicit selected imports.

status: Result<Int, Result.Error> = Result.Ok(0);

emptyCanonical: STD.Optional.Optional<Str> = STD.Optional.None;
canonicalName: STD.Optional.Optional<Str> = STD.Optional.Some("Leitwolf");
shortName: Optional<Str> = canonicalName;

display: Str = name ?? "guest";
name ??= STD.Optional.Some("guest");

// Automatic unboxing from Optional<T> to T may fail. New code models that as
// Result.Error rather than legacy exceptions.

unboxedName: Str = name;
forcedName: Str = name!;

user: Optional<User> = None;
city: Optional<Str> = user?.Address?.City;

fn NormalizeOptionalName(value: Optional<Str>) => Result.Result<Str, Result.Error> {
    if (value == None) {
        return Result.Error(Result.Error {
            message: "name is missing",
        });
    }

    return Result.Ok(value!);
}
