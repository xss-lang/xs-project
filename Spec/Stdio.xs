// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Stdio module:

//
// Stdio provides formatted text output macros, formatted writer macros, and
// standard stream handles.
// print!, println!, eprint!, eprintln!, write!, writeln!, and format! are
// exported Stdio macros.
// They are available through `imports Stdio`, not as compiler built-ins.
// format_args! is a built-in macro, not a Stdio export.
//
// Stdio does not provide filesystem operations.
// Filesystem operations belong to the FS module.
//
// Stdio does not provide string interpolation.
// Formatted output uses placeholder-based formatting.
//

imports Stdio, FS, Collections;


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

// writer macros

fn WriteToStream() throws IOException {
    write!(STD.Stdout, "Hello");
}

fn WriteLineToStream() throws IOException {
    writeln!(STD.Stdout, "{} is {}", "Alpha", 26);
}

fn WriteNewlineOnlyToStream() throws IOException {
    writeln!(STD.Stdout);
}

// formatting

fn FormatValues() throws IOException {
    user: Str = "Alpha";
    age: Int = 26;

    println!("{} is {}", user, age);
}

// print!, eprint!, and format! require a Str format template as the first
// argument.
//
// println! and eprintln! either take no arguments or use the same format
// template form as print!/eprint!. With no arguments they write exactly one
// newline. With arguments they append exactly one newline after the formatted
// text. This matches Rust 1.57 output macro behavior.
//
// write! requires a destination expression followed by a Str format template.
// writeln! either takes only a destination expression or a destination followed
// by the same format template form. With only a destination it writes exactly
// one newline. With a template it appends exactly one newline after the
// formatted text. This matches Rust 1.57 writer macro behavior.
//
// "{}" formats one value with the Display formatter.
// "{:?}" formats one value with the Debug formatter.
// "{:#?}" formats one value with the pretty Debug formatter.
// "{{" emits "{".
// "}}" emits "}".
//
// Placeholder count must match the number of following arguments.


// format! returns Str and does not write to a stream.
// Built-in format_args! returns the formatting argument value used by output
// and writer macros and does not write to a stream.

fn BuildMessage() {
    user1: Str = "Alpha";
    user2: Str = "Leitwolf";

    users: Str = format!("{} {}", user1, user2);
    format_args!("{} {}", user1, user2);
}


// standard stream handles

fn StandardHandles() throws IOException {
    STD.FS.write(STD.Stdout, "stdout text\n");
    STD.FS.write(STD.Stderr, "stderr text\n");

    text: Str = STD.FS.readToStr(STD.Stdin());
    println!("{}", text);
}

// STD.Stdout and STD.Stderr are stream handles.
// STD.Stdin() returns the standard input stream handle.
// Raw reading and writing through these handles is provided by STD.FS.
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

// INVALID: built-in format_args! requires a Str format template.
