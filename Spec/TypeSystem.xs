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
// Users may import Optional explicitly, but normal source files do not need to.
// Optional<T> is not lowered through the enum data mechanism.
// There is no nullable T? type operator.

name: Optional<Str> = None;
name = Some("Leitwolf");

display: Str = name ?? "guest";
name ??= Some("guest");

// Automatic unboxing from Optional<T> to T may throw
// OptionalUnboxingException when the value is None.
// Other Optional runtime failures use OptionalException.

unboxedName: Str = name;
forcedName: Str = name!;

user: Optional<User> = None;
city: Optional<Str> = user?.Address?.City;
