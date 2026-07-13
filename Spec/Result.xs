// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Result module:

//
// Result provides explicit success/error values.
// The legacy exception syntax remains parseable, but it is deprecated.
// New code should use Result<T, E> and postfix @ propagation instead of
// throws, throw, try, catch, and finally.
//

imports fs, stdio;


// result data model

enum data Result<T, E> {
    Ok: T,
    Error: E,
}

data Error {
    message: Str,
}


// success construction

fn Succeed() -> Result<(), Error> {
    return Ok(());
}


// propagation

fn Propagate() -> Result<(), Error> {
    DoWork()@;
    return Ok(());
}

fn DoWork() -> Result<(), Error> {
    return Ok(());
}

// The postfix @ operator propagates Error from the current function.
// On Ok(value), it evaluates to value.
// On Error(error), it returns that error from the enclosing function.


// explicit match without @

fn ReadFileExplicit(path: Str) -> Result<Optional<Str>, Error> {
    file = match (std::fs::File::open(path)) {
        Ok(value) -> value,
        Error(error) -> return Error(error),
    };

    content: Optional<Str> = std::optional::Some("");

    match (file.read_to_string(&mut content)) {
        Ok(else) -> {},
        Error(error) -> return Error(error),
    }

    return Ok(content);
}


// propagation with @

fn ReadFile(path: Str) -> Result<Optional<Str>, Error> {
    file = std::fs::File::open(path)@;
    content: Optional<Str> = std::optional::Some("");
    file.read_to_string(&mut content)@;
    return Ok(content);
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
