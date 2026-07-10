// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// variable system:

//
// Type annotations are required unless the declaration uses `:=`.
// Normal variables are mutable.
// Top-level variables are not supported.
//

fn Main() {
    // mutable variables
    x: Int = 5;
    name: Str = "Alpha";
    enabled: Bool = true;
    letter: Char = "A";
    data: Byte = 255;

    small: Short = 10;
    normal: Long = 20;
    large: Int = 30;
    huge: Integer = 40;

    usmall: UShort = 10;
    unormal: ULong = 20;
    ularge: UInt = 30;
    uhuge: UInteger = 40;

    single: SFloat = 2.5;
    floating: Float = 3.5;

    // Type-inferred bindings use := and always have an initializer.
    inferredAge := 26;
    inferredPi := 3.14;
    inferredLetter := 'A';
    inferredName := "Alpha";
    maybe: Optional<Int> = None;

    // reassignment
    x = 10;
    name = "Leitewolf";
    enabled = false;

    // immutable variable
    val version: Str = "1.0";
    val maxPlayers: Int = 100;
    val isReady: Bool = true;
    val inferredVersion := "1.0";

    // compile-time constant
    const BuildNumber: Int = 42;
    const AppName: Str = "Example";
    const DebugMode: Bool = false;
    const inferredBuildNumber := 42;

    // compile-time constant + static lifetime
    static CacheSize: Int = 1024;
    static ProductName: Str = "Example";
    static IsProduction: Bool = true;
}


// VALID
fn Main() {
    x: Int = 5;
}


// INVALID
fn Main() {
    x = 5;
}
// = is assignment, not a type-inferred declaration.


// INVALID
fn Main() {
    missing := None;
}
// Optional<T> is never inferred. Write the Optional<T> annotation explicitly.


// VALID
fn Main() {
    val x: Int = 5;
}


// INVALID
fn Main() {
    val x: Int = 5;
    x = 10;
}
// Cannot assign to immutable variable.


// VALID
fn Main() {
    const Version: Int = 1;
}


// INVALID
fn Main() {
    const Version: Int = RuntimeValue();
}
// Const values must be compile-time constants.


// VALID
fn Main() {
    static BufferSize: Int = 4096;
}


// INVALID
fn Main() {
    static BufferSize: Int = RuntimeValue();
}
// Static values must be compile-time constants.


// INVALID
Version: Int = 1;
// Top-level variables are not allowed.
