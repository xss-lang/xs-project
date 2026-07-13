// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Panic module:

//
// Panic exports assertion and panic macros.
//
// The compiler behaves as if `imports panic;` existed. The following macros
// are available without an explicit import, through explicit `imports panic`,
// or through imports panic or using namespace panic. They are not compiler built-ins:
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

// imports panic; is optional.


fn validate_positive(value: Long) {
    assert!(value > 0);
}


fn validate_pair(left: Long, right: Long) {
    assert_eq!(left, right);
    assert_ne!(left, 0);
}


fn debug_validate(index: Long, length: Long) {
    debug_assert!(index >= 0);
    debug_assert_eq!(length, length);
}


fn stop_now() {
    panic!();
}


fn stop_with_message(name: Str) {
    panic!("invalid name: {}", name);
}


// Selected imports are valid:

using namespace panic;

fn selected_panic_macros(value: Long) {
    assert!(value != 0);
    panic!("namespace using example");
}
