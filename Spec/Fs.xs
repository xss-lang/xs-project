// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Fs module:

//
// Fs provides filesystem and raw stream operations.
//
// Formatted output belongs to Stdio.
// Fs.Write does not format text and does not add a newline.
//

imports Fs, Stdio, Collections;


// raw writes

fn WriteText() throws IOException {
    Fs.Write("log.txt", "hello\n");
    Fs.Write(std.Stdout, "stdout\n");
    Fs.Write(std.Stderr, "stderr\n");
}

// Fs.Write(path, text):
//
// - Writes the exact Str bytes/text representation selected by the runtime.
// - Does not apply formatting.
// - Does not append a newline.
// - Requires the file to exist.
// - Throws IOException if the file does not exist or cannot be written.


// raw reads

fn ReadText() throws IOException {
    content: Str = Fs.ReadToStr("log.txt");
    println!("{}", content);
}

fn ReadBytes() throws IOException {
    bytes: Collections.vector<Byte> = Fs.Read("image.png");
    println!("file size: {}", bytes.length());
}

// Fs.ReadToStr(path-or-stream) reads UTF-16 text into Str.
// Fs.Read(path-or-stream) reads raw bytes into Collections.vector<Byte>.


// create paths

fn CreatePaths() throws IOException {
    Fs.CreateDir("data/backups");
    Fs.CreateFile("data/backups/log.txt");
}

// Fs.CreateDir(path):
//
// - Creates the directory and missing parent directories.
// - Succeeds if the directory already exists.
// - Throws IOException if a non-directory path component blocks creation.
//
// Fs.CreateFile(path):
//
// - Creates a new empty file.
// - Throws IOException if the file already exists.
// - Throws IOException if the parent directory does not exist.


// move, copy and remove

fn MoveCopyRemove() throws IOException {
    Fs.CopyFile("data/source.txt", "data/copy.txt");
    Fs.Move("data/copy.txt", "data/backups/copy.txt");
    Fs.RemoveFile("data/source.txt");

    Fs.CopyDir("data/backups", "data/archive");
    Fs.RemoveDir("data/archive");
}

// Fs.RemoveDir removes an empty directory.
// Recursive directory deletion is intentionally not the default operation.


// list directory

fn ListDirectory() throws IOException {
    for (entry: Str in Fs.ListDir(".")) {
        println!("{}", entry.trim());
    }
}

// Fs.ListDir(path) returns Collections.vector<Str>.
// Entries are returned in deterministic lexicographic order.


// metadata and path helpers

fn InspectPath(path: Str) throws IOException {
    if (Fs.Exists(path) && Fs.IsDir(path)) {
        for (entry: Str in Fs.WalkDir(path)) {
            relative: Str = Fs.RelativePath(path, entry);
            println!("{} {}", relative, Fs.Size(entry));
        }
    }
}

fn BuildPath(root: Str, name: Str) => Str {
    path: Str = Fs.JoinPath(root, name);
    return Fs.ParentDir(path);
}

// Fs.Exists(path) returns false instead of throwing for a missing path.
// Fs.IsFile(path) and Fs.IsDir(path) return false for a missing path.
// Fs.Size(path) returns the byte length of a regular file.
// Fs.ModifiedTicks(path) returns a runtime-defined comparable timestamp.
// Fs.JoinPath(base, child) joins path components using the target platform.
// Fs.ParentDir(path) returns the lexical parent directory.
// Fs.RelativePath(root, path) returns a lexical relative path.
// Fs.WalkDir(path) recursively returns files and directories in
// deterministic lexicographic order.
// Fs.WalkDir does not follow symbolic links by default.


// file handles and open options

fn OpenWithOptions() throws IOException {
    file: Fs.File = Fs.OpenOptions.new()
        .create(true)
        .append(true)
        .open("log.txt");

    Fs.Write(file, "new log line\n");
}

// Fs.File is an owned file handle.
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
