// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Reads an application log and prints level counts plus the newest error line.

module Programs.LogAggregator;

imports Collections, FS, Optional, Stdio, Process;

enum data LogError {
    Io: IOException,
    InvalidLine: Str,
}

data LogEntry {
    level: Str;
    message: Str;
}

class LogParser {
    static fn Parse(line: Str) => LogEntry throws LogError {
        parts: STD.Collections.vector<Str> = line.split(" ", 2);
        if (parts.length() != 2) {
            throw LogError.InvalidLine(line);
        }

        return LogEntry {
            level: parts[0].trim(),
            message: parts[1].trim(),
        };
    }
}

class Report {
    counts: STD.Collections.hash_map<Str, Int>;
    newestError: Optional<Str>;

    Report() {
        this.counts = STD.Collections.hash_map<Str, Int>.new();
        this.newestError = STD.Optional.None;
    }

    fn Add(entry: LogEntry) {
        current: Int = this.counts.get(entry.level).unwrapOr(0);
        this.counts[entry.level] = current + 1;

        if (entry.level == "ERROR") {
            this.newestError = STD.Optional.Some(entry.message);
        }
    }

    fn Print() throws IOException {
        for ((level, count): (Str, Int) in this.counts) {
            println!("{:<8} {}", level, count);
        }

        if (this.newestError != None) {
            println!("newest error: {}", this.newestError!);
        }
    }
}

fn Main(args: STD.Process.Args) => Int throws LogError, IOException {
    if (args.length() != 2) {
        eprintln!("usage: log-aggregator <log-file>");
        return 2;
    }

    report: Report = new();
    content: Str = STD.FS.readToStr(args[1]);

    for (line: Str in content.lines()) {
        if (line.trim().length() == 0) {
            continue;
        }

        report.Add(LogParser.Parse(line));
    }

    report.Print();
    return 0;
}
