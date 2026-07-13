// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Recursively copies changed files from one directory into a backup directory.

module Programs::FileBackup;

imports collections, stdio, fs, process, result;

enum data BackupError {
    Io: IOException,
    InvalidSource: Str,
}

data FileEntry {
    path: Str;
    relative: Str;
    bytes: Int;
    modified_ticks: Int;
}

class BackupPlan {
    sourceRoot: Str;
    targetRoot: Str;
    files: std::collections::vector<FileEntry>;

    BackupPlan(sourceRoot: Str, targetRoot: Str) {
        this.sourceRoot = sourceRoot;
        this.targetRoot = targetRoot;
        this.files = std::collections::vector<FileEntry>::new();
    }

    fn Discover() -> Result<(), BackupError> {
        if (!std::fs::is_dir(this.sourceRoot)) {
            return Error(BackupError::InvalidSource(this.sourceRoot));
        }

        for (path: Str in std::fs::walk_dir(this.sourceRoot)) {
            if (std::fs::is_dir(path)) {
                continue;
            }

            relative: Str = std::fs::relative_path(this.sourceRoot, path);
            this.files.push(FileEntry {
                path: path,
                relative: relative,
                bytes: std::fs::size(path),
                modified_ticks: std::fs::modified_ticks(path),
            });
        }
        return Ok();
    }

    fn Execute() -> Result<(), IOException> {
        for (entry: FileEntry in this.files) {
            destination: Str = std::fs::join_path(this.targetRoot, entry.relative);

            if (this.ShouldCopy(entry, destination)) {
                std::fs::create_dir(std::fs::parent_dir(destination));
                std::fs::copy_file(entry.path, destination);
                println!("copied {}", entry.relative);
            }
        }
        return Ok();
    }

    fn ShouldCopy(entry: FileEntry, destination: Str) -> Bool {
        if (!std::fs::exists(destination)) {
            return true;
        }

        return std::fs::size(destination) != entry.bytes || std::fs::modified_ticks(destination) < entry.modified_ticks;
    }
}

fn Main(args: std::collections::vector<Str>) -> Result<Int, Error> {
    if (args.length() != 3) {
        eprintln!("usage: backup <source> <target>");
        return 2;
    }

    plan: BackupPlan = new(args[1], args[2]);
    plan.Discover()@;
    plan.Execute()@;
    return Ok(0);
}
