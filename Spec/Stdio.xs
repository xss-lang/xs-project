// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Stdio module:

//
// Stdio provides formatted text output macros and standard stream handles.
// print!, println!, eprint!, eprintln!, format! and format_args! are exported
// Stdio macros.
// They are available through `imports Stdio`, not as compiler built-ins.
//
// Stdio does not provide filesystem operations.
// Filesystem operations belong to the Fs module.
//
// Stdio does not provide string interpolation.
// Formatted output uses placeholder-based formatting.
//

imports Stdio, Fs, Collections;


// standard output macros

fn PrintWithoutNewline() throws IOException {
    print!("Hello");
}

fn PrintWithNewline() throws IOException {
    println!("Hello");
}

fn PrintNewlineOnly() throws IOException {
    println!();
}


// standard error macros

fn ErrorWithoutNewline() throws IOException {
    eprint!("error");
}

fn ErrorWithNewline() throws IOException {
    eprintln!("error");
}

fn ErrorNewlineOnly() throws IOException {
    eprintln!();
}


// formatting

fn FormatValues() throws IOException {
    user: Str = "Alpha";
    age: Int = 26;

    println!("{} is {}", user, age);
}

// print!, eprint!, format! and format_args! require a Str format template as
// the first argument.
//
// println! and eprintln! either take no arguments or use the same format
// template form as print!/eprint!. With no arguments they write exactly one
// newline. With arguments they append exactly one newline after the formatted
// text. This matches Rust 1.57 output macro behavior.
//
// "{}" formats one value with the Display formatter.
// "{:?}" formats one value with the Debug formatter.
// "{{" emits "{".
// "}}" emits "}".
//
// Placeholder count must match the number of following arguments.


// format! returns Str and does not write to a stream.
// format_args! returns the Stdio formatting argument value used by output
// macros and does not write to a stream.

fn BuildMessage() {
    user1: Str = "Alpha";
    user2: Str = "Leitwolf";

    users: Str = format!("{} {}", user1, user2);
    format_args!("{} {}", user1, user2);
}


// standard stream handles

fn StandardHandles() throws IOException {
    STD.Fs.write(STD.Stdout, "stdout text\n");
    STD.Fs.write(STD.Stderr, "stderr text\n");

    text: Str = STD.Fs.readToStr(STD.Stdin());
    println!("{}", text);
}

// STD.Stdout and STD.Stderr are stream handles.
// STD.Stdin() returns the standard input stream handle.
// Raw reading and writing through these handles is provided by STD.Fs.
// Stdio macros use STD.Stdout and STD.Stderr internally.


// line input

fn ReadLine() {
    input: Optional<Str> = Some("");

    STD.Stdin()
        .readLine(&mut input)
        .expect("input could not be read");

    println!("Input: {}", input.trim());
}

fn ReadNumber() {
    input: Optional<Str> = Some("");

    STD.Stdin()
        .readLine(&mut input)
        .expect("input could not be read");

    number: Int = input.trim().parse().expect("expected a valid number");
    println!("Number: {}", number);
}

fn ReadManyNumbers() {
    input: Optional<Str> = Some("");

    STD.Stdin()
        .readLine(&mut input)
        .unwrap();

    numbers: STD.Collections.vector<Int> = input
        .splitWhitespace()
        .map(fn(value) {
            return value.parse().expect("invalid number");
        });

    println!("Input: {:?}", numbers);
}


// invalid examples

fn InvalidNonStringTemplate() throws IOException {
    println!(10);
}

// INVALID: println! expects a Str format template as its first argument.


fn InvalidMissingPlaceholder() throws IOException {
    println!("value", 10);
}

// INVALID: one argument is supplied but the template has no placeholder.


fn InvalidMissingArgument() throws IOException {
    println!("{}",);
}

// INVALID: the template has one placeholder but no value argument.


fn InvalidEmptyPrint() throws IOException {
    print!();
}

// INVALID: print! has no newline-only form.


fn InvalidEmptyFormatArgs() {
    format_args!();
}

// INVALID: format_args! requires a Str format template.
