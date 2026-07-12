// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// FS module:

//
// STD.FS provides filesystem and raw stream operations.
//
// Formatted output belongs to Stdio.
// STD.FS.write does not format text and does not add a newline.
//

imports FS, Stdio, Collections, Result;


// raw writes

fn WriteText() => Result.Result<Void, IOException> {
    STD.FS.write("log.txt", "hello\n");
    STD.FS.write(STD.Stdout, "stdout\n");
    STD.FS.write(STD.Stderr, "stderr\n");
}

// STD.FS.write(path, text):
//
// - Writes the exact Str bytes/text representation selected by the runtime.
// - Does not apply formatting.
// - Does not append a newline.
// - Requires the file to exist.
// - Returns Result.Error(IOException) if the file does not exist or cannot be written.


// raw reads

fn ReadText() => Result.Result<Void, IOException> {
    content: Str = STD.FS.readToStr("log.txt");
    println!("{}", content);
}

fn ReadBytes() => Result.Result<Void, IOException> {
    bytes: STD.Collections.vector<Byte> = STD.FS.read("image.png");
    println!("file size: {}", bytes.length());
}

// STD.FS.readToStr(path-or-stream) reads UTF-16 text into Str.
// STD.FS.read(path-or-stream) reads raw bytes into STD.Collections.vector<Byte>.


// create paths

fn CreatePaths() => Result.Result<Void, IOException> {
    STD.FS.createDir("data/backups");
    STD.FS.createFile("data/backups/log.txt");
}

// STD.FS.createDir(path):
//
// - Creates the directory and missing parent directories.
// - Succeeds if the directory already exists.
// - Returns Result.Error(IOException) if a non-directory path component blocks creation.
//
// STD.FS.createFile(path):
//
// - Creates a new empty file.
// - Returns Result.Error(IOException) if the file already exists.
// - Returns Result.Error(IOException) if the parent directory does not exist.


// move, copy and remove

fn MoveCopyRemove() => Result.Result<Void, IOException> {
    STD.FS.copyFile("data/source.txt", "data/copy.txt");
    STD.FS.move("data/copy.txt", "data/backups/copy.txt");
    STD.FS.removeFile("data/source.txt");

    STD.FS.copyDir("data/backups", "data/archive");
    STD.FS.removeDir("data/archive");
}

// STD.FS.removeDir removes an empty directory.
// Recursive directory deletion is intentionally not the default operation.


// list directory

fn ListDirectory() => Result.Result<Void, IOException> {
    for (entry: Str in STD.FS.listDir(".")) {
        println!("{}", entry.trim());
    }
}

// STD.FS.listDir(path) returns STD.Collections.vector<Str>.
// Entries are returned in deterministic lexicographic order.


// metadata and path helpers

fn InspectPath(path: Str) => Result.Result<Void, IOException> {
    if (STD.FS.exists(path) && STD.FS.isDir(path)) {
        for (entry: Str in STD.FS.walkDir(path)) {
            relative: Str = STD.FS.relativePath(path, entry);
            println!("{} {}", relative, STD.FS.size(entry));
        }
    }
}

fn BuildPath(root: Str, name: Str) => Str {
    path: Str = STD.FS.joinPath(root, name);
    return STD.FS.parentDir(path);
}

// STD.FS.exists(path) returns false instead of throwing for a missing path.
// STD.FS.isFile(path) and STD.FS.isDir(path) return false for a missing path.
// STD.FS.size(path) returns the byte length of a regular file.
// STD.FS.modifiedTicks(path) returns a runtime-defined comparable timestamp.
// STD.FS.joinPath(base, child) joins path components using the target platform.
// STD.FS.parentDir(path) returns the lexical parent directory.
// STD.FS.relativePath(root, path) returns a lexical relative path.
// STD.FS.walkDir(path) recursively returns files and directories in
// deterministic lexicographic order.
// STD.FS.walkDir does not follow symbolic links by default.


// file handles and open options

fn OpenWithOptions() => Result.Result<Void, IOException> {
    file: STD.FS.File = STD.FS.OpenOptions.new()
        .create(true)
        .append(true)
        .open("log.txt");

    STD.FS.write(file, "new log line\n");
}

// STD.FS.File is an owned file handle.
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
