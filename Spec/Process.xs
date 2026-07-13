// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Process module and error handling:

//
// Process execution is provided by std::process::
//
// New X# code uses Result<T, E> for recoverable process and I/O
// failures. The old exception syntax remains parseable for legacy code, but it
// is deprecated and should not be used in new examples.
//
// Raw shell execution is disabled by default. Shell execution must be
// explicitly enabled with .shell(true).
//

imports process, result, std;


// process execution

fn PullRepository() -> Result<Void, Error> {
    std::process::execute("git", ["pull"])@;
    return Ok();
}


// process execution with arguments

fn InstallPackage(name: Str) -> Result<Void, Error> {
    std::process::execute("pacman", ["-S", name])@;
    return Ok();
}


// shell execution

fn UpgradeSystem() -> Result<Void, Error> {
    std::process::execute("pacman -Syu")
        .shell(true)@;

    return Ok();
}


// INVALID

fn InvalidShellExecution() -> Result<Void, Error> {
    std::process::execute("pacman -Syu")@;
    return Ok();
}

// shell execution requires .shell(true)


// explicit Result match

fn PrintGitStatus() -> Result<Void, Error> {
    status = match (std::process::execute("git", ["status"])) {
        Ok(value) -> value,
        Error(error) -> return Error(error),
    };

    println!("{:?}", status);
    return Ok();
}


// propagation with @

fn RunFormatter() -> Result<Void, Error> {
    std::process::execute("xsfmt", ["--check"])@;
    std::process::execute("xstidy", ["--check"])@;
    return Ok();
}


// custom error model

enum data LoginError {
    InvalidUser: Str,
    Locked: Str,
}

fn Login(user: Str) -> Result<Void, LoginError> {
    if (user.length() == 0) {
        return Error(LoginError.InvalidUser(user));
    }

    if (user == "root") {
        return Error(LoginError.Locked(user));
    }

    return Ok();
}


// built-in error-style values

fn ReadRequiredLine() -> Result<Str, Error> {
    input: Optional<Str> = std::optional::Some("");

    std::stdin()
        .read_line(&mut input)@;

    if (input == None) {
        return Error(Error {
            message: "no input",
        });
    }

    return Ok(input!);
}


// deterministic cleanup still applies

class File {
    File.Drop() {
        println!("File closed");
    }
}

fn UseFile() -> Result<Void, Error> {
    file: File = new();
    std::process::execute("git", ["rev-parse", "--is-inside-work-tree"])@;
    return Ok();
}

// File.Drop() runs before returning Error from @ propagation.


// legacy exception syntax

// Deprecated syntax remains parseable and produces compiler warnings:
//
// fn OpenFile(path: Str) throws IOException {
//     throw IOException("File not found");
// }
//
// try {
//     OpenFile("missing.txt");
// }
// catch (error: IOException) {
// }
//
// New code should use Result<T, E> instead.
