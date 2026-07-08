// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Recursively copies changed files from one directory into a backup directory.

module Programs.FileBackup;

imports Collections, Stdio, Process;

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
    files: Collections.vector<FileEntry>;

    BackupPlan(sourceRoot: Str, targetRoot: Str) {
        this.sourceRoot = sourceRoot;
        this.targetRoot = targetRoot;
        this.files = Collections.vector<FileEntry>.new();
    }

    fn Discover() throws BackupError {
        if (!std.isDirectory(this.sourceRoot)) {
            throw BackupError.InvalidSource(this.sourceRoot);
        }

        for (path: Str in std.walk(this.sourceRoot)) {
            if (std.isDirectory(path)) {
                continue;
            }

            relative: Str = std.relativePath(this.sourceRoot, path);
            this.files.push(FileEntry {
                path: path,
                relative: relative,
                bytes: std.size(path),
                modifiedTicks: std.modifiedTicks(path),
            });
        }
    }

    fn Execute() throws IOException {
        for (entry: FileEntry in this.files) {
            destination: Str = std.joinPath(this.targetRoot, entry.relative);

            if (this.ShouldCopy(entry, destination)) {
                std.createDirectory(std.parentDirectory(destination));
                std.copyFile(entry.path, destination);
                std.cout << "copied " << entry.relative << "\n";
            }
        }
    }

    fn ShouldCopy(entry: FileEntry, destination: Str) => Bool {
        if (!std.exists(destination)) {
            return true;
        }

        return std.size(destination) != entry.bytes || std.modifiedTicks(destination) < entry.modifiedTicks;
    }
}

fn Main(args: Collections.vector<Str>) => Int throws BackupError, IOException {
    if (args.length() != 3) {
        std.cerr << "usage: backup <source> <target>\n";
        return 2;
    }

    plan: BackupPlan = new(args[1], args[2]);
    plan.Discover();
    plan.Execute();
    return 0;
}
