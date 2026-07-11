// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Panic module:

//
// Panic exports assertion and panic macros.
//
// The following macros are available through `imports Panic` or selected
// imports from Panic. They are not compiler built-ins:
//
// - assert!
// - assert_eq!
// - assert_ne!
// - debug_assert!
// - debug_assert_eq!
// - panic!
//
// Their source-level meanings follow Rust's macros with the same names.
//
// assert!(condition) panics when condition is false.
// assert_eq!(left, right) panics when left != right.
// assert_ne!(left, right) panics when left == right.
// debug_assert!(condition) is enabled in debug builds and may be disabled
// in optimized release builds.
// debug_assert_eq!(left, right) follows debug_assert! with equality.
// panic!() unconditionally starts panic propagation.
//
// Panic formatting follows the same placeholder-based formatting model as
// Stdio formatting. Runtime formatting and unwinding ABI are implementation
// details of the Panic runtime.

imports Panic;


fn ValidatePositive(value: Long) {
    assert!(value > 0);
}


fn ValidatePair(left: Long, right: Long) {
    assert_eq!(left, right);
    assert_ne!(left, 0);
}


fn DebugValidate(index: Long, length: Long) {
    debug_assert!(index >= 0);
    debug_assert_eq!(length, length);
}


fn StopNow() {
    panic!();
}


fn StopWithMessage(name: Str) {
    panic!("invalid name: {}", name);
}


// Selected imports are valid:

from Panic imports assert, panic;

fn SelectedPanicMacros(value: Long) {
    assert!(value != 0);
    panic!("selected import example");
}
