// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Result module:

//
// Result provides explicit success/error values. Recoverable failures use
// Result<T> or Result<T, E>, and postfix @ propagates an Error value.
//

imports fs, stdio;


// result data model

// The compiler provides Result<T, E> and its Ok(T)/Error(E) variants. User
// source does not redeclare that enum data type.

data Error {
    message: Str,
}

// Result<T> is the canonical shorthand for Result<T, Error>.
// Unit success is written as Result<()>, not Result<(), Error>.
// Result is not the default return type. A function with no written return type
// returns unit. Other functions may declare another type. A body that uses postfix `@`, constructs
// `Ok(...)`, or constructs `Error(...)` must declare a Result return type.


// success construction

fn succeed() -> Result<()> {
    return Ok(());
}


// propagation

fn propagate() -> Result<()> {
    do_work()@;
    return Ok(());
}

fn do_work() -> Result<()> {
    return Ok(());
}

// The postfix @ operator propagates Error from the current function.
// On Ok(value), it evaluates to value.
// On Error(error), it returns that error from the enclosing function.


// explicit match without @

fn read_file_explicit(path: Str) -> Result<Optional<Str>, Error> {
    file = match (std::fs::File::open(path)) {
        Ok(value) -> value,
        Error(error) -> return Error(error),
    };

    content: Optional<Str> = Some("");

    match (file.read_to_string(&mut content)) {
        Ok(else) -> {},
        Error(error) -> return Error(error),
    }

    return Ok(content);
}


// propagation with @

fn read_file(path: Str) -> Result<Optional<Str>, Error> {
    file = std::fs::File::open(path)@;
    content: Optional<Str> = Some("");
    file.read_to_string(&mut content)@;
    return Ok(content);
}


// expect and unwrap

fn read_required_line() -> Str {
    line: Optional<Str> = Some("");

    std::stdin()
        .read_line(&mut line)
        .expect("input could not be read");

    return line.unwrap();
}

// expect(message) unwraps the success value or panics with message.
// unwrap() unwraps the success value or panics with the default Result panic.
