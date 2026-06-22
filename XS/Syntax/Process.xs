// process and error handling system:

//{
Process execution is provided by the Process module.

Process.execute() does not return a value.

Command execution failures throw exceptions.

Raw shell execution is disabled by default.

Shell execution must be explicitly enabled
using .shell(true).

Error handling is based on:

- try
- catch
- finally
- throw
- throws

Functions may declare thrown exceptions.

Multiple exception types may be declared.

Exceptions are normal classes.

Built-in exceptions:

- IOException
- NetworkException
- NilException
- ArgumentException
- OutOfRangeException

Users may define custom exception types.

During exception unwinding:

- RAII rules apply
- Destructors run automatically
- Destructors execute in LIFO order
}//

imports Process;


// process execution

Process.execute(
    "git"
    ["pull"]
);


// process execution with arguments

Process.execute(
    "pacman"
    ["-S", "foo"]
);


// shell execution

Process.execute(
    "pacman -Syu"
).shell(true);


// INVALID

Process.execute(
    "pacman -Syu"
);

// shell execution requires .shell(true)


// exceptions

try {
    Process.execute(
        "git"
        ["pull"]
    );
}
catch (e: IOException) {
}
finally {
}


// multiple catch blocks

try {
    Process.execute(
        "git"
        ["pull"]
    );
}
catch (e: IOException) {
}
catch (e: NetworkException) {
}
finally {
}


// throw

throw IOException("File not found");


// throws

fn OpenFile(path: str)
throws IOException {
}


// multiple throws

fn Connect()
throws IOException, NetworkException {
}


// custom exception

class LoginException {
    message: str;
}


// throwing custom exceptions

throw LoginException {
    message: "Invalid username or password",
};


// catching custom exceptions

try {
    Login();
}
catch (e: LoginException) {
}


// exception propagation

fn Login()
throws LoginException {

    throw LoginException {
        message: "Invalid username or password",
    };
}


// built-in exceptions

throw IOException("Disk error");

throw NetworkException("Connection failed");

throw NilException("Unexpected nil");

throw ArgumentException("Invalid argument");

throw OutOfRangeException("Index out of range");


// RAII during exception unwinding

class File {
    File.Drop() {
        std.cout << "File closed\n";
    }
}

fn Test()
throws IOException {

    file: File = new();

    throw IOException("Boom");
}

// File.Drop() runs automatically


// LIFO destruction order

class Resource {
    Resource.Drop() {
    }
}

fn Test()
throws IOException {

    a: Resource = new();
    b: Resource = new();
    c: Resource = new();

    throw IOException("Boom");
}

// destruction order:
//
// c.Drop()
// b.Drop()
// a.Drop()


// VALID

try {
}
catch (e: IOException) {
}


// VALID

try {
}
catch (e: IOException) {
}
catch (e: NetworkException) {
}
finally {
}


// VALID

fn OpenFile(path: str)
throws IOException {
}


// VALID

fn Connect()
throws IOException, NetworkException {
}


// VALID

class LoginException {
    message: str;
}


// VALID

throw LoginException {
    message: "Login failed",
};


// INVALID

Process.execute(
    "git pull"
);

// shell execution requires .shell(true)


// INVALID

Process.execute(
    "git pull"
).shell(false);

// shell execution requires .shell(true)


// INVALID

catch (e) {
}

// catch type is required


// INVALID

fn OpenFile(path: str) {
    throw IOException("File not found");
}

// IOException must be declared in throws
