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
        self.sourceRoot = sourceRoot;
        self.targetRoot = targetRoot;
        self.files = std::collections::vector<FileEntry>::new();
    }

    fn Discover() -> Result<(), BackupError> {
        if (!std::fs::is_dir(self.sourceRoot)) {
            return Error(BackupError::InvalidSource(self.sourceRoot));
        }

        for (path: Str in std::fs::walk_dir(self.sourceRoot)) {
            if (std::fs::is_dir(path)) {
                continue;
            }

            relative: Str = std::fs::relative_path(self.sourceRoot, path);
            self.files.push(FileEntry {
                path: path,
                relative: relative,
                bytes: std::fs::size(path),
                modified_ticks: std::fs::modified_ticks(path),
            });
        }
        return Ok();
    }

    fn Execute() -> Result<(), IOException> {
        for (entry: FileEntry in self.files) {
            destination: Str = std::fs::join_path(self.targetRoot, entry.relative);

            if (self.ShouldCopy(entry, destination)) {
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
