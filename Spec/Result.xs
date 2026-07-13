// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Result module:

//
// Result provides explicit success/error values.
// The legacy exception syntax remains parseable, but it is deprecated.
// New code should use Result::Result<T, E> and postfix @ propagation instead of
// throws, throw, try, catch, and finally.
//

imports result, fs, stdio;


// result data model

enum data Result<T, E> {
    Ok: T,
    Error: E,
}

data Error {
    message: Str,
}


// success construction

fn Succeed() -> Result::Result<(), Result::Error> {
    return Result::Ok(());
}


// propagation

fn Propagate() -> Result::Result<(), Result::Error> {
    DoWork()@;
    return Result::Ok(());
}

fn DoWork() -> Result::Result<(), Result::Error> {
    return Result::Ok(());
}

// The postfix @ operator propagates Result::Error from the current function.
// On Result::Ok(value), it evaluates to value.
// On Result::Error(error), it returns that error from the enclosing function.


// explicit match without @

fn ReadFileExplicit(path: Str) -> Result::Result<Optional<Str>, Result::Error> {
    file = match (std::fs::File.open(path)) {
        Result::Ok(value) -> value,
        Result::Error(error) -> return Result::Error(error),
    };

    content: Optional<Str> = std::optional::Some("");

    match (file.read_to_string(&mut content)) {
        Result::Ok(else) -> {},
        Result::Error(error) -> return Result::Error(error),
    }

    return Result::Ok(content);
}


// propagation with @

fn ReadFile(path: Str) -> Result::Result<Optional<Str>, Result::Error> {
    file = std::fs::File.open(path)@;
    content: Optional<Str> = std::optional::Some("");
    file.read_to_string(&mut content)@;
    return Result::Ok(content);
}


// expect and unwrap

fn ReadRequiredLine() -> Str {
    line: Optional<Str> = std::optional::Some("");

    std::stdin()
        .read_line(&mut line)
        .expect("input could not be read");

    return line.unwrap();
}

// expect(message) unwraps the success value or panics with message.
// unwrap() unwraps the success value or panics with the default Result panic.
