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
    files: Collections.vector<FileEntry>;

    BackupPlan(sourceRoot: Str, targetRoot: Str) {
        this.sourceRoot = sourceRoot;
        this.targetRoot = targetRoot;
        this.files = Collections.vector<FileEntry>.new();
    }

    fn Discover() throws BackupError {
        if (!Fs.IsDir(this.sourceRoot)) {
            throw BackupError.InvalidSource(this.sourceRoot);
        }

        for (path: Str in Fs.WalkDir(this.sourceRoot)) {
            if (Fs.IsDir(path)) {
                continue;
            }

            relative: Str = Fs.RelativePath(this.sourceRoot, path);
            this.files.push(FileEntry {
                path: path,
                relative: relative,
                bytes: Fs.Size(path),
                modifiedTicks: Fs.ModifiedTicks(path),
            });
        }
    }

    fn Execute() throws IOException {
        for (entry: FileEntry in this.files) {
            destination: Str = Fs.JoinPath(this.targetRoot, entry.relative);

            if (this.ShouldCopy(entry, destination)) {
                Fs.CreateDir(Fs.ParentDir(destination));
                Fs.CopyFile(entry.path, destination);
                println!("copied {}", entry.relative);
            }
        }
    }

    fn ShouldCopy(entry: FileEntry, destination: Str) => Bool {
        if (!Fs.Exists(destination)) {
            return true;
        }

        return Fs.Size(destination) != entry.bytes || Fs.ModifiedTicks(destination) < entry.modifiedTicks;
    }
}

fn Main(args: Collections.vector<Str>) => Int throws BackupError, IOException {
    if (args.length() != 3) {
        eprintln!("usage: backup <source> <target>");
        return 2;
    }

    plan: BackupPlan = new(args[1], args[2]);
    plan.Discover();
    plan.Execute();
    return 0;
}
