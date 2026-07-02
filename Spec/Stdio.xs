// Stdio system:

//{
The Stdio module provides:

- Standard input
- Standard output
- Standard error
- File creation
- Directory creation
- File opening
- File reading
- File writing
- File and directory checks
- Basic file information

std.format is not provided by Stdio.
It belongs to the separate Format module.
}//

imports Stdio;


// standard output

fn StandardOutput() throws IOException {
    std.cout << "Hello\n";

    value: int = 42;

    std.cout
        << "Value: "
        << value
        << "\n";
}


// standard input

fn StandardInput() throws IOException {
    number: int;

    std.cout << "Enter a number: ";
    std.cin >> number;

    std.cout
        << "Entered number: "
        << number
        << "\n";
}


// standard error

fn StandardError() throws IOException {
    std.cerr << "An error occurred\n";
}


// standard streams through fout and fin

fn StandardStreamEquivalents() throws IOException {
    value: int;

    // Equivalent to std.cout
    std.fout << [stdout] << "Hello\n";

    // Equivalent to std.cerr
    std.fout << [stderr] << "Error\n";

    // Equivalent to std.cin
    std.fin >> [stdin] >> value;
}


// file creation

fn CreateFile() throws IOException {
    std.fcreate("file.txt");
}

// std.fcreate():
//
// - Creates a new file.
// - Does not open the file.
// - Does not return a value.
// - Throws IOException if the file already exists.
// - Throws IOException if the parent directory does not exist.


// directory creation

fn CreateDirectory() throws IOException {
    std.dcreate("folder");
}

// std.dcreate():
//
// - Creates a new directory.
// - Does not return a value.
// - Throws IOException if the directory already exists.
// - Does not automatically create missing parent directories.


// file opening

fn OpenFile() throws IOException {
    openedFile: file = std.fopen("file.txt");
}

// std.fopen():
//
// - Opens an existing file.
// - Always opens the file for reading and writing.
// - Does not support an explicit mode argument.
// - Throws IOException if the file does not exist.
// - Throws IOException if the path points to a directory.


// file lifetime

// Open file handles are closed automatically by their destructor.
//
// Manual Close() is not required.
//
// Destruction follows the ownership, RAII and LIFO rules.


// file output

fn WriteFile() throws IOException {
    openedFile: file = std.fopen("file.txt");

    std.fout << [openedFile] << "Written";
}

// std.fout always appends to the end of the file.
// It does not truncate or overwrite existing contents.


// chained file output

fn ChainedFileOutput() throws IOException {
    openedFile: file = std.fopen("file.txt");

    age: int = 26;

    std.fout
        << [openedFile]
        << "Age: "
        << age
        << "\n";
}


// output target position

// The [target] expression may appear at any position in the chain.

fn OutputTargetPosition() throws IOException {
    openedFile: file = std.fopen("file.txt");

    std.fout << [openedFile] << "A";
    std.fout << "B" << [openedFile];
}


// multiple output targets

// <<< changes the output target.

fn MultipleOutputTargets() throws IOException {
    openedFile: file = std.fopen("file.txt");

    std.fout
        << [stdout]
        << "Normal output\n"
        <<< [stderr]
        << "Error output\n"
        <<< [openedFile]
        << "File output\n";
}

// Equivalent to:
//
// std.fout << [stdout] << "Normal output\n";
// std.fout << [stderr] << "Error output\n";
// std.fout << [openedFile] << "File output\n";


// file input

fn ReadFile() throws IOException {
    openedFile: file = std.fopen("file.txt");

    text: str;

    std.fin >> [openedFile] >> text;
}

// std.fin reads the entire file contents.
// After reading, the file cursor remains at the end.


// read and return cursor to the beginning

fn ReadAndReset() throws IOException {
    openedFile: file = std.fopen("file.txt");

    text: str;

    std.fin
        >> [openedFile]
        >> text
        >> "\r";
}

// "\r" moves the file cursor back to the beginning
// when used at the end of a std.fin chain.
//
// "\r" remains the carriage-return escape sequence
// and may also be used elsewhere.


// chained input

fn ChainedInput() throws IOException {
    openedFile: file = std.fopen("file.txt");

    name: str;
    age: int;

    std.fin
        >> [openedFile]
        >> name
        >> age;
}


// input target position

// The [target] expression may appear at any position in the chain.

fn InputTargetPosition() throws IOException {
    openedFile: file = std.fopen("file.txt");

    name: str;
    age: int;

    std.fin >> [openedFile] >> name >> age;
    std.fin >> name >> [openedFile] >> age;
    std.fin >> name >> age >> [openedFile];
}


// multiple input targets

// >>> changes the input target.

fn MultipleInputTargets() throws IOException {
    firstFile: file = std.fopen("first.txt");
    secondFile: file = std.fopen("second.txt");

    name: str;
    age: int;

    std.fin
        >> [firstFile]
        >> name
        >>> [secondFile]
        >> age;
}

// Equivalent to:
//
// std.fin >> [firstFile] >> name;
// std.fin >> [secondFile] >> age;


// stdin and file input in one chain

fn MixedInputTargets() throws IOException {
    openedFile: file = std.fopen("file.txt");

    name: str;
    content: str;

    std.fin
        >> [stdin]
        >> name
        >>> [openedFile]
        >> content;
}


// file and directory checks

fn PathChecks() throws IOException {
    openedFile: file = std.fopen("file.txt");

    fileCheck: bool = std.isFile(openedFile);
    filePathCheck: bool = std.isFile("file.txt");

    directoryCheck: bool = std.isDir("folder");
    openedFileDirectoryCheck: bool = std.isDir(openedFile);

    pathExists: bool = std.exists("file.txt");
    openedFileExists: bool = std.exists(openedFile);
}

// std.isFile(), std.isDir() and std.exists():
//
// - Accept a path string or an opened file value.
// - Return true when the check succeeds positively.
// - Return false when the check succeeds negatively.
// - Return nil when an error occurs.
//
// std.exists() works with both files and directories.


// open-state check

fn OpenState() throws IOException {
    openedFile: file = std.fopen("file.txt");

    state: bool = std.isOpen(openedFile);
}

// std.isOpen():
//
// - Returns true if the file is open.
// - Returns false if the file is not open.
// - Returns nil if the check fails.


// file name

fn FileName() throws IOException {
    openedFile: file = std.fopen("file.txt");

    name: str = std.fname(openedFile);
}

// std.fname():
//
// - Returns only the file name.
//
// Example result:
//
// "file.txt"
//
// - Does not return the full path.
// - Does not throw when the operation fails.
// - Returns "" when the operation fails.


// file size

fn FileSize() throws IOException, OutOfRangeException {
    openedFile: file = std.fopen("file.txt");

    size: byte = std.fsize(openedFile);
}

// std.fsize():
//
// - Returns the file size as byte.
// - The size is measured in bytes.
// - Returns nil when the operation fails.
// - Throws OutOfRangeException if the size exceeds
//   the range supported by byte.


// escape sequences

//{
\'          -> Single quote
\"          -> Double quote
\\          -> Backslash
\0          -> Null character
\a          -> Alert
\b          -> Backspace
\f          -> Form feed
\n          -> New line
\r          -> Carriage return
\t          -> Horizontal tab
\v          -> Vertical tab
\uXXXX      -> Four-digit Unicode escape
\UXXXXXXXX  -> Eight-digit Unicode escape
\xXX        -> Hexadecimal character escape
}//


// escape-sequence examples

fn EscapeSequences() throws IOException {
    std.cout << "First line\nSecond line\n";
    std.cout << "Name:\tAlfa\n";
    std.cout << "\"Quoted text\"\n";
    std.cout << "Backslash: \\\n";
    std.cout << "\u0041\n";
}


// VALID

fn ValidConsoleIO() throws IOException {
    value: int;

    std.cout << "Value: ";
    std.cin >> value;
}


// VALID

fn ValidFileIO() throws IOException {
    openedFile: file = std.fopen("file.txt");

    text: str;

    std.fin >> [openedFile] >> text >> "\r";

    std.fout
        << [openedFile]
        << "Read content: "
        << text
        << "\n";
}


// VALID

fn ValidTargetSwitching() throws IOException {
    firstFile: file = std.fopen("first.txt");
    secondFile: file = std.fopen("second.txt");

    std.fout
        << [firstFile]
        << "First"
        <<< [secondFile]
        << "Second";
}


// INVALID

fn InvalidOpenMode() {
    openedFile: file =
        std.fopen("file.txt", "read");
}

// std.fopen() does not accept a mode argument.


// INVALID

fn InvalidCreateResult() {
    createdFile: file =
        std.fcreate("file.txt");
}

// std.fcreate() does not return a value.


// INVALID

fn InvalidDirectoryResult() {
    createdDirectory: file =
        std.dcreate("folder");
}

// std.dcreate() does not return a value.


// INVALID

fn InvalidSecondInputTarget() throws IOException {
    firstFile: file = std.fopen("first.txt");
    secondFile: file = std.fopen("second.txt");

    name: str;
    age: int;

    std.fin
        >> [firstFile]
        >> name
        >> [secondFile]
        >> age;
}

// A new input target requires >>>.


// INVALID

fn InvalidSecondOutputTarget() throws IOException {
    firstFile: file = std.fopen("first.txt");
    secondFile: file = std.fopen("second.txt");

    std.fout
        << [firstFile]
        << "First"
        << [secondFile]
        << "Second";
}

// A new output target requires <<<.


// Stdio does not provide:
//
// - File deletion
// - Directory deletion
// - File renaming
// - File moving
// - Full-path retrieval
// - File-position retrieval
// - File-modification-time retrieval
// - Direct file truncation
// - Direct overwrite mode
//
// File deletion, directory deletion, renaming,
// moving and overwrite workflows use the Process API.
