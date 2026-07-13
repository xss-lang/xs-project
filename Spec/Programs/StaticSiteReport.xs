// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Scans a content directory and emits a small static-site health report.

module Programs::StaticSiteReport;

imports collections, fs, optional, stdio, process, result;

enum data SiteError {
    Io: IOException,
    MissingTitle: Str,
}

data PageInfo {
    path: Str;
    title: Str;
    wordCount: Int;
}

class Markdown {
    static fn Title(path: Str, text: Str) -> Result::Result<Str, SiteError> {
        for (line: Str in text.lines()) {
            if (line.startsWith("# ")) {
                return Result::Ok(line.trimStart("# ").trim());
            }
        }

        return Result::Error(SiteError.MissingTitle(path));
    }

    static fn CountWords(text: Str) -> Int {
        return text.split_whitespace().length();
    }
}

class SiteReport {
    pages: std::collections::vector<PageInfo>;

    SiteReport() {
        this.pages = std::collections::vector<PageInfo>.new();
    }

    fn AddMarkdown(path: Str) -> Result::Result<Void, Result::Error> {
        text: Str = std::fs::read_to_str(path);
        this.pages.push(PageInfo {
            path: path,
            title: Markdown.Title(path, text)@,
            wordCount: Markdown.CountWords(text),
        });
        return Result::Ok();
    }

    fn Print() -> Result::Result<Void, IOException> {
        println!("pages: {}", this.pages.length());

        for (page: PageInfo in this.pages) {
            println!("{:<32} {:>6} {}", page.title, page.wordCount, page.path);
        }
        return Result::Ok();
    }
}

fn Main(args: std::process::Args) -> Result::Result<Int, Result::Error> {
    root: Str = if (args.length() == 2) {
        args[1];
    }
    else {
        ".";
    };

    report: SiteReport = new();
    for (path: Str in std::fs::walk(root)) {
        if (path.endsWith(".md")) {
            report.AddMarkdown(path)@;
        }
    }

    report.Print()@;
    return Result::Ok(0);
}
