// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Recursively copies changed files from one directory into a backup directory.

module Programs.FileBackup;

imports Collections, Stdio, Fs, Process;

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

    fn Discover() throws BackupError {
        if (!STD.Fs.isDir(this.sourceRoot)) {
            throw BackupError.InvalidSource(this.sourceRoot);
        }

        for (path: Str in STD.Fs.walkDir(this.sourceRoot)) {
            if (STD.Fs.isDir(path)) {
                continue;
            }

            relative: Str = STD.Fs.relativePath(this.sourceRoot, path);
            this.files.push(FileEntry {
                path: path,
                relative: relative,
                bytes: STD.Fs.size(path),
                modifiedTicks: STD.Fs.modifiedTicks(path),
            });
        }
    }

    fn Execute() throws IOException {
        for (entry: FileEntry in this.files) {
            destination: Str = STD.Fs.joinPath(this.targetRoot, entry.relative);

            if (this.ShouldCopy(entry, destination)) {
                STD.Fs.createDir(STD.Fs.parentDir(destination));
                STD.Fs.copyFile(entry.path, destination);
                println!("copied {}", entry.relative);
            }
        }
    }

    fn ShouldCopy(entry: FileEntry, destination: Str) => Bool {
        if (!STD.Fs.exists(destination)) {
            return true;
        }

        return STD.Fs.size(destination) != entry.bytes || STD.Fs.modifiedTicks(destination) < entry.modifiedTicks;
    }
}

fn Main(args: STD.Collections.vector<Str>) => Int throws BackupError, IOException {
    if (args.length() != 3) {
        eprintln!("usage: backup <source> <target>");
        return 2;
    }

    plan: BackupPlan = new(args[1], args[2]);
    plan.Discover();
    plan.Execute();
    return 0;
}
