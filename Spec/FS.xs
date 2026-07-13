// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// FS module:

//
// std::fs provides filesystem and raw stream operations.
//
// Formatted output belongs to Stdio.
// std::fs::write does not format text and does not add a newline.
//

imports fs, stdio, collections, result;


// raw writes

fn WriteText() -> Result<(), IOException> {
    std::fs::write("log.txt", "hello\n");
    std::fs::write(std::stdout, "stdout\n");
    std::fs::write(std::stderr, "stderr\n");
}

// std::fs::write(path, text):
//
// - Writes the exact Str bytes/text representation selected by the runtime.
// - Does not apply formatting.
// - Does not append a newline.
// - Requires the file to exist.
// - Returns Error(IOException) if the file does not exist or cannot be written.


// raw reads

fn ReadText() -> Result<(), IOException> {
    content: Str = std::fs::read_to_str("log.txt");
    println!("{}", content);
}

fn ReadBytes() -> Result<(), IOException> {
    bytes: std::collections::vector<Byte> = std::fs::read("image.png");
    println!("file size: {}", bytes.length());
}

// std::fs::read_to_str(path-or-stream) reads UTF-16LE/UTF-16BE text into Str.
// std::fs::read(path-or-stream) reads raw bytes into std::collections::vector<Byte>.


// create paths

fn CreatePaths() -> Result<(), IOException> {
    std::fs::create_dir("data/backups");
    std::fs::create_file("data/backups/log.txt");
}

// std::fs::create_dir(path):
//
// - Creates the directory and missing parent directories.
// - Succeeds if the directory already exists.
// - Returns Error(IOException) if a non-directory path component blocks creation.
//
// std::fs::create_file(path):
//
// - Creates a new empty file.
// - Returns Error(IOException) if the file already exists.
// - Returns Error(IOException) if the parent directory does not exist.


// move, copy and remove

fn MoveCopyRemove() -> Result<(), IOException> {
    std::fs::copy_file("data/source.txt", "data/copy.txt");
    std::fs::move("data/copy.txt", "data/backups/copy.txt");
    std::fs::remove_file("data/source.txt");

    std::fs::copy_dir("data/backups", "data/archive");
    std::fs::remove_dir("data/archive");
}

// std::fs::remove_dir removes an empty directory.
// Recursive directory deletion is intentionally not the default operation.


// list directory

fn ListDirectory() -> Result<(), IOException> {
    for (entry: Str in std::fs::list_dir(".")) {
        println!("{}", entry.trim());
    }
}

// std::fs::list_dir(path) returns std::collections::vector<Str>.
// Entries are returned in deterministic lexicographic order.


// metadata and path helpers

fn InspectPath(path: Str) -> Result<(), IOException> {
    if (std::fs::exists(path) && std::fs::is_dir(path)) {
        for (entry: Str in std::fs::walk_dir(path)) {
            relative: Str = std::fs::relative_path(path, entry);
            println!("{} {}", relative, std::fs::size(entry));
        }
    }
}

fn BuildPath(root: Str, name: Str) -> Str {
    path: Str = std::fs::join_path(root, name);
    return std::fs::parent_dir(path);
}

// std::fs::exists(path) returns false instead of throwing for a missing path.
// std::fs::is_file(path) and std::fs::is_dir(path) return false for a missing path.
// std::fs::size(path) returns the byte length of a regular file.
// std::fs::modified_ticks(path) returns a runtime-defined comparable timestamp.
// std::fs::join_path(base, child) joins path components using the target platform.
// std::fs::parent_dir(path) returns the lexical parent directory.
// std::fs::relative_path(root, path) returns a lexical relative path.
// std::fs::walk_dir(path) recursively returns files and directories in
// deterministic lexicographic order.
// std::fs::walk_dir does not follow symbolic links by default.


// file handles and open options

fn OpenWithOptions() -> Result<(), IOException> {
    file: std::fs::File = std::fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open("log.txt");

    std::fs::write(file, "new log line\n");
}

// std::fs::File is an owned file handle.
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
