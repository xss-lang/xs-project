// variable system:

//{
Type annotation zorunludur.
Normal değişkenler mutable'dır.
Top-level variables desteklenmez.
}//

fn Main() {
    // mutable variables
    x: int = 5;
    name: str = "Alfa";
    enabled: bool = true;
    letter: char = "A";
    data: byte = 255;

    small: int16 = 10;
    normal: int32 = 20;
    large: int = 30;
    huge: int128 = 40;

    usmall: uint16 = 10;
    unormal: uint32 = 20;
    ularge: uint = 30;
    uhuge: uint128 = 40;

    half: float16 = 1.5;
    single: float32 = 2.5;
    doublePrecision: float = 3.5;
    quadPrecision: double = 4.5;

    // reassignment
    x = 10;
    name = "Hasan";
    enabled = false;

    // immutable variable
    val version: str = "1.0";
    val maxPlayers: int = 100;
    val isReady: bool = true;

    // compile-time constant
    const BuildNumber: int = 42;
    const AppName: str = "Example";
    const DebugMode: bool = false;

    // compile-time constant + static lifetime
    static CacheSize: int = 1024;
    static ProductName: str = "Example";
    static IsProduction: bool = true;
}


// VALID
fn Main() {
    x: int = 5;
}


// INVALID
fn Main() {
    x = 5;
}
// Type annotation is required.


// VALID
fn Main() {
    val x: int = 5;
}


// INVALID
fn Main() {
    val x: int = 5;
    x = 10;
}
// Cannot assign to immutable variable.


// VALID
fn Main() {
    const Version: int = 1;
}


// INVALID
fn Main() {
    const Version: int = RuntimeValue();
}
// Const values must be compile-time constants.


// VALID
fn Main() {
    static BufferSize: int = 4096;
}


// INVALID
fn Main() {
    static BufferSize: int = RuntimeValue();
}
// Static values must be compile-time constants.


// INVALID
Version: int = 1;
// Top-level variables are not allowed.
