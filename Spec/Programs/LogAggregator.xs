// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Reads an application log and prints level counts plus the newest error line.

module programs::log_aggregator;

imports collections, fs, optional, stdio, process;

data LogEntry {
    level: Str;
    message: Str;
}

class LogParser {
    static fn parse(line: Str) -> Result<LogEntry, Error> {
        parts: std::collections::Vector<Str> = line.split(" ", 2);
        if (parts.length() != 2) {
            return Error(new Error("invalid log line"));
        }

        return Ok(LogEntry {
            level: parts[0].trim(),
            message: parts[1].trim(),
        });
    }
}

class Report {
    counts: std::collections::HashMap<Str, Int>;
    newest_error: Optional<Str>;

    Report() {
        self.counts = std::collections::HashMap<Str, Int>::new();
        self.newest_error = None;
    }

    fn add(entry: LogEntry) {
        current: Int = self.counts.get(entry.level).unwrap_or(0);
        self.counts[entry.level] = current + 1;

        if (entry.level == "ERROR") {
            self.newest_error = Some(entry.message);
        }
    }

    fn print() -> Result<()> {
        for ((level, count): (Str, Int) in self.counts) {
            println!("{:<8} {}", level, count);
        }

        if (self.newest_error != None) {
            println!("newest error: {}", self.newest_error!);
        }

        return Ok();
    }
}

fn main(args: std::process::Args) -> Result<Int, Error> {
    if (args.length() != 2) {
        eprintln!("usage: log-aggregator <log-file>");
        return 2;
    }

    report: Report = new Report();
    content: Str = std::fs::read_to_str(args[1]);

    for (line: Str in content.lines()) {
        if (line.trim().length() == 0) {
            continue;
        }

        report.add(LogParser::parse(line)@);
    }

    report.print()@;
    return Ok(0);
}
