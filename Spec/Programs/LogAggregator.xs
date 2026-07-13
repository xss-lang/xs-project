// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Reads an application log and prints level counts plus the newest error line.

module Programs::LogAggregator;

imports collections, fs, optional, stdio, process, result;

enum data LogError {
    Io: IOException,
    InvalidLine: Str,
}

data LogEntry {
    level: Str;
    message: Str;
}

class LogParser {
    static fn Parse(line: Str) -> Result::Result<LogEntry, LogError> {
        parts: std::collections::vector<Str> = line.split(" ", 2);
        if (parts.length() != 2) {
            return Result::Error(LogError.InvalidLine(line));
        }

        return Result::Ok(LogEntry {
            level: parts[0].trim(),
            message: parts[1].trim(),
        });
    }
}

class Report {
    counts: std::collections::hash_map<Str, Int>;
    newestError: Optional<Str>;

    Report() {
        this.counts = std::collections::hash_map<Str, Int>.new();
        this.newestError = std::optional::None;
    }

    fn Add(entry: LogEntry) {
        current: Int = this.counts.get(entry.level).unwrapOr(0);
        this.counts[entry.level] = current + 1;

        if (entry.level == "ERROR") {
            this.newestError = std::optional::Some(entry.message);
        }
    }

    fn Print() -> Result::Result<Void, IOException> {
        for ((level, count): (Str, Int) in this.counts) {
            println!("{:<8} {}", level, count);
        }

        if (this.newestError != None) {
            println!("newest error: {}", this.newestError!);
        }

        return Result::Ok();
    }
}

fn Main(args: std::process::Args) -> Result::Result<Int, Result::Error> {
    if (args.length() != 2) {
        eprintln!("usage: log-aggregator <log-file>");
        return 2;
    }

    report: Report = new();
    content: Str = std::fs::read_to_str(args[1]);

    for (line: Str in content.lines()) {
        if (line.trim().length() == 0) {
            continue;
        }

        report.Add(LogParser.Parse(line)@);
    }

    report.Print()@;
    return Result::Ok(0);
}
