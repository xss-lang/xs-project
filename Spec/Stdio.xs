// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Stdio module:

//
// Stdio provides formatted text output macros, formatted writer macros, and
// standard stream functions.
// print!, println!, eprint!, eprintln!, write!, writeln!, and format! are
// exported Stdio macros.
// They are available through `imports stdio`, not as compiler built-ins.
// format_args! is a built-in macro, not a Stdio export.
//
// Stdio does not provide filesystem operations.
// Filesystem operations belong to the FS module.
//
// Stdio does not provide string interpolation.
// Formatted output uses placeholder-based formatting.
//

imports stdio, fs, collections, result;


// standard output macros

fn PrintWithoutNewline() -> Result<()> {
    print!("Hello");
}

fn PrintWithNewline() -> Result<()> {
    println!("Hello");
}

fn PrintNewlineOnly() -> Result<()> {
    println!();
}


// standard error macros

fn ErrorWithoutNewline() -> Result<()> {
    eprint!("error");
}

fn ErrorWithNewline() -> Result<()> {
    eprintln!("error");
}

fn ErrorNewlineOnly() -> Result<()> {
    eprintln!();
}

// writer macros

fn WriteToStream() -> Result<()> {
    write!(std::stdout(), "Hello");
}

fn WriteLineToStream() -> Result<()> {
    writeln!(std::stdout(), "{} is {}", "Alpha", 26);
}

fn WriteNewlineOnlyToStream() -> Result<()> {
    writeln!(std::stdout());
}

// formatting

fn FormatValues() -> Result<()> {
    user: Str = "Alpha";
    age: Int = 26;

    println!("{} is {}", user, age);
    println!("{:?}", user);
    println!("{:#?}", user);
    println!("{:_>12}", user);
    println!("{:08x}", age);
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
    debugUsers := format_args!("{:#?}", users);
    aligned: Str = format!("{:^16}", user1);
    hexAge: Str = format!("{:#x}", 26);
}

fn WriteReportLine(name: Str, score: Int) -> Result<()> {
    writeln!(std::stdout(), "{:<16} {:>4}", name, score);
}

fn WriteDebugReport<T>(value: T) -> Result<()> {
    write!(std::stderr(), "{:#?}", value);
    writeln!(std::stderr());
}


// standard stream functions

fn StandardHandles() -> Result<()> {
    std::fs::write(std::stdout(), "stdout text\n");
    std::fs::write(std::stderr(), "stderr text\n");

    text: Str = std::fs::read_to_str(std::stdin());
    println!("{}", text);
}

// std::stdout() and std::stderr() return standard output and error stream handles.
// std::stdin() returns the standard input stream handle.
// Raw reading and writing through these handles is provided by std::fs::
// Stdio macros use std::stdout() and std::stderr() internally.


// line input

fn ReadLine() {
    input: Optional<Str> = std::optional::Some("");

    std::stdin()
        .read_line(&mut input)
        .expect("input could not be read");

    println!("Input: {}", input.trim());
}

fn ReadNumber() {
    input: Optional<Str> = std::optional::Some("");

    std::stdin()
        .read_line(&mut input)
        .expect("input could not be read");

    number: Int = input.trim().parse().expect("expected a valid number");
    println!("Number: {}", number);
}

fn ReadManyNumbers() {
    input: Optional<Str> = std::optional::Some("");

    std::stdin()
        .read_line(&mut input)
        .unwrap();

    numbers: std::collections::Vector<Int> = input
        .split_whitespace()
        .map(fn(value) {
            return value.parse().expect("invalid number");
        });

    println!("Input: {:?}", numbers);
}


// invalid examples

fn InvalidNonStringTemplate() -> Result<()> {
    println!(10);
}

// INVALID: println! expects a Str format template as its first argument.


fn InvalidMissingPlaceholder() -> Result<()> {
    println!("value", 10);
}

// INVALID: one argument is supplied but the template has no placeholder.


fn InvalidMissingArgument() -> Result<()> {
    println!("{}",);
}

// INVALID: the template has one placeholder but no value argument.


fn InvalidEmptyPrint() -> Result<()> {
    print!();
}

// INVALID: print! has no newline-only form.


fn InvalidEmptyFormatArgs() {
    format_args!();
}

// INVALID: built-in format_args! requires a Str format template.
