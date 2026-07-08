// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// variable system:

//{
Type annotation zorunludur.
Normal değişkenler mutable'dır.
Top-level variables desteklenmez.
}//

fn Main() {
    // mutable variables
    x: Int = 5;
    name: Str = "Alfa";
    enabled: Bool = true;
    letter: Char = "A";
    data: Byte = 255;

    small: int16 = 10;
    normal: int32 = 20;
    large: Int = 30;
    huge: int128 = 40;

    usmall: uint16 = 10;
    unormal: uint32 = 20;
    ularge: UInt = 30;
    uhuge: uint128 = 40;

    single: float32 = 2.5;
    floating: Float = 3.5;

    // reassignment
    x = 10;
    name = "Hasan";
    enabled = false;

    // immutable variable
    val version: Str = "1.0";
    val maxPlayers: Int = 100;
    val isReady: Bool = true;

    // compile-time constant
    const BuildNumber: Int = 42;
    const AppName: Str = "Example";
    const DebugMode: Bool = false;

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
// Type annotation is required.


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
