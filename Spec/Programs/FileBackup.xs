// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Recursively copies changed files from one directory into a backup directory.

module Programs.FileBackup;

imports Collections, Stdio, FS, Process, Result;

enum data BackupError {
    Io: IOException,
    InvalidSource: Str,
}

data FileEntry {
    path: Str;
    relative: Str;
    bytes: Int;
    modifiedTicks: Int;
}

class BackupPlan {
    sourceRoot: Str;
    targetRoot: Str;
    files: STD.Collections.vector<FileEntry>;

    BackupPlan(sourceRoot: Str, targetRoot: Str) {
        this.sourceRoot = sourceRoot;
        this.targetRoot = targetRoot;
        this.files = STD.Collections.vector<FileEntry>.new();
    }

    fn Discover() => Result.Result<Void, BackupError> {
        if (!STD.FS.isDir(this.sourceRoot)) {
            return Result.Error(BackupError.InvalidSource(this.sourceRoot));
        }

        for (path: Str in STD.FS.walkDir(this.sourceRoot)) {
            if (STD.FS.isDir(path)) {
                continue;
            }

            relative: Str = STD.FS.relativePath(this.sourceRoot, path);
            this.files.push(FileEntry {
                path: path,
                relative: relative,
                bytes: STD.FS.size(path),
                modifiedTicks: STD.FS.modifiedTicks(path),
            });
        }
        return Result.Ok();
    }

    fn Execute() => Result.Result<Void, IOException> {
        for (entry: FileEntry in this.files) {
            destination: Str = STD.FS.joinPath(this.targetRoot, entry.relative);

            if (this.ShouldCopy(entry, destination)) {
                STD.FS.createDir(STD.FS.parentDir(destination));
                STD.FS.copyFile(entry.path, destination);
                println!("copied {}", entry.relative);
            }
        }
        return Result.Ok();
    }

    fn ShouldCopy(entry: FileEntry, destination: Str) => Bool {
        if (!STD.FS.exists(destination)) {
            return true;
        }

        return STD.FS.size(destination) != entry.bytes || STD.FS.modifiedTicks(destination) < entry.modifiedTicks;
    }
}

fn Main(args: STD.Collections.vector<Str>) => Result.Result<Int, Result.Error> {
    if (args.length() != 3) {
        eprintln!("usage: backup <source> <target>");
        return 2;
    }

    plan: BackupPlan = new(args[1], args[2]);
    plan.Discover()@;
    plan.Execute()@;
    return Result.Ok(0);
}
