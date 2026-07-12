// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Fs module:

//
// STD.Fs provides filesystem and raw stream operations.
//
// Formatted output belongs to Stdio.
// STD.Fs.write does not format text and does not add a newline.
//

imports Fs, Stdio, Collections;


// raw writes

fn WriteText() throws IOException {
    STD.Fs.write("log.txt", "hello\n");
    STD.Fs.write(STD.Stdout, "stdout\n");
    STD.Fs.write(STD.Stderr, "stderr\n");
}

// STD.Fs.write(path, text):
//
// - Writes the exact Str bytes/text representation selected by the runtime.
// - Does not apply formatting.
// - Does not append a newline.
// - Requires the file to exist.
// - Throws IOException if the file does not exist or cannot be written.


// raw reads

fn ReadText() throws IOException {
    content: Str = STD.Fs.readToStr("log.txt");
    println!("{}", content);
}

fn ReadBytes() throws IOException {
    bytes: STD.Collections.vector<Byte> = STD.Fs.read("image.png");
    println!("file size: {}", bytes.length());
}

// STD.Fs.readToStr(path-or-stream) reads UTF-16 text into Str.
// STD.Fs.read(path-or-stream) reads raw bytes into STD.Collections.vector<Byte>.


// create paths

fn CreatePaths() throws IOException {
    STD.Fs.createDir("data/backups");
    STD.Fs.createFile("data/backups/log.txt");
}

// STD.Fs.createDir(path):
//
// - Creates the directory and missing parent directories.
// - Succeeds if the directory already exists.
// - Throws IOException if a non-directory path component blocks creation.
//
// STD.Fs.createFile(path):
//
// - Creates a new empty file.
// - Throws IOException if the file already exists.
// - Throws IOException if the parent directory does not exist.


// move, copy and remove

fn MoveCopyRemove() throws IOException {
    STD.Fs.copyFile("data/source.txt", "data/copy.txt");
    STD.Fs.move("data/copy.txt", "data/backups/copy.txt");
    STD.Fs.removeFile("data/source.txt");

    STD.Fs.copyDir("data/backups", "data/archive");
    STD.Fs.removeDir("data/archive");
}

// STD.Fs.removeDir removes an empty directory.
// Recursive directory deletion is intentionally not the default operation.


// list directory

fn ListDirectory() throws IOException {
    for (entry: Str in STD.Fs.listDir(".")) {
        println!("{}", entry.trim());
    }
}

// STD.Fs.listDir(path) returns STD.Collections.vector<Str>.
// Entries are returned in deterministic lexicographic order.


// metadata and path helpers

fn InspectPath(path: Str) throws IOException {
    if (STD.Fs.exists(path) && STD.Fs.isDir(path)) {
        for (entry: Str in STD.Fs.walkDir(path)) {
            relative: Str = STD.Fs.relativePath(path, entry);
            println!("{} {}", relative, STD.Fs.size(entry));
        }
    }
}

fn BuildPath(root: Str, name: Str) => Str {
    path: Str = STD.Fs.joinPath(root, name);
    return STD.Fs.parentDir(path);
}

// STD.Fs.exists(path) returns false instead of throwing for a missing path.
// STD.Fs.isFile(path) and STD.Fs.isDir(path) return false for a missing path.
// STD.Fs.size(path) returns the byte length of a regular file.
// STD.Fs.modifiedTicks(path) returns a runtime-defined comparable timestamp.
// STD.Fs.joinPath(base, child) joins path components using the target platform.
// STD.Fs.parentDir(path) returns the lexical parent directory.
// STD.Fs.relativePath(root, path) returns a lexical relative path.
// STD.Fs.walkDir(path) recursively returns files and directories in
// deterministic lexicographic order.
// STD.Fs.walkDir does not follow symbolic links by default.


// file handles and open options

fn OpenWithOptions() throws IOException {
    file: STD.Fs.File = STD.Fs.OpenOptions.new()
        .create(true)
        .append(true)
        .open("log.txt");

    STD.Fs.write(file, "new log line\n");
}

// STD.Fs.File is an owned file handle.
// File handles close through deterministic drop.
// OpenOptions defaults:
//
// - read(false)
// - write(true)
// - create(false)
// - append(false)
// - truncate(false)
//
// append(true) writes at the end.
// truncate(true) clears existing file contents when opening for write.
