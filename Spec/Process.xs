// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Process module and error handling:

//
// Process execution is provided by std.Process.
//
// New X# code uses Result.Result<T, E> for recoverable process and I/O
// failures. The old exception syntax remains parseable for legacy code, but it
// is deprecated and should not be used in new examples.
//
// Raw shell execution is disabled by default. Shell execution must be
// explicitly enabled with .shell(true).
//

imports process, result, std;


// process execution

fn PullRepository() => Result.Result<Void, Result.Error> {
    std.Process.execute("git", ["pull"])@;
    return Result.Ok();
}


// process execution with arguments

fn InstallPackage(name: Str) => Result.Result<Void, Result.Error> {
    std.Process.execute("pacman", ["-S", name])@;
    return Result.Ok();
}


// shell execution

fn UpgradeSystem() => Result.Result<Void, Result.Error> {
    std.Process.execute("pacman -Syu")
        .shell(true)@;

    return Result.Ok();
}


// INVALID

fn InvalidShellExecution() => Result.Result<Void, Result.Error> {
    std.Process.execute("pacman -Syu")@;
    return Result.Ok();
}

// shell execution requires .shell(true)


// explicit Result match

fn PrintGitStatus() => Result.Result<Void, Result.Error> {
    status = match (std.Process.execute("git", ["status"])) {
        Result.Ok(value) -> value,
        Result.Error(error) -> return Result.Error(error),
    };

    println!("{:?}", status);
    return Result.Ok();
}


// propagation with @

fn RunFormatter() => Result.Result<Void, Result.Error> {
    std.Process.execute("xsfmt", ["--check"])@;
    std.Process.execute("xstidy", ["--check"])@;
    return Result.Ok();
}


// custom error model

enum data LoginError {
    InvalidUser: Str,
    Locked: Str,
}

fn Login(user: Str) => Result.Result<Void, LoginError> {
    if (user.length() == 0) {
        return Result.Error(LoginError.InvalidUser(user));
    }

    if (user == "root") {
        return Result.Error(LoginError.Locked(user));
    }

    return Result.Ok();
}


// built-in error-style values

fn ReadRequiredLine() => Result.Result<Str, Result.Error> {
    input: Optional<Str> = std.optional.Some("");

    std.stdin()
        .read_line(&mut input)@;

    if (input == None) {
        return Result.Error(Result.Error {
            message: "no input",
        });
    }

    return Result.Ok(input!);
}


// deterministic cleanup still applies

class File {
    File.Drop() {
        println!("File closed");
    }
}

fn UseFile() => Result.Result<Void, Result.Error> {
    file: File = new();
    std.Process.execute("git", ["rev-parse", "--is-inside-work-tree"])@;
    return Result.Ok();
}

// File.Drop() runs before returning Result.Error from @ propagation.


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
// New code should use Result.Result<T, E> instead.
