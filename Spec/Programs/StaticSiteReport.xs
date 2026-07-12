// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Scans a content directory and emits a small static-site health report.

module Programs.StaticSiteReport;

imports Collections, FS, Optional, Stdio, Process;

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
    static fn Title(path: Str, text: Str) => Str throws SiteError {
        for (line: Str in text.lines()) {
            if (line.startsWith("# ")) {
                return line.trimStart("# ").trim();
            }
        }

        throw SiteError.MissingTitle(path);
    }

    static fn CountWords(text: Str) => Int {
        return text.splitWhitespace().length();
    }
}

class SiteReport {
    pages: STD.Collections.vector<PageInfo>;

    SiteReport() {
        this.pages = STD.Collections.vector<PageInfo>.new();
    }

    fn AddMarkdown(path: Str) throws SiteError, IOException {
        text: Str = STD.FS.readToStr(path);
        this.pages.push(PageInfo {
            path: path,
            title: Markdown.Title(path, text),
            wordCount: Markdown.CountWords(text),
        });
    }

    fn Print() throws IOException {
        println!("pages: {}", this.pages.length());

        for (page: PageInfo in this.pages) {
            println!("{:<32} {:>6} {}", page.title, page.wordCount, page.path);
        }
    }
}

fn Main(args: STD.Process.Args) => Int throws SiteError, IOException {
    root: Str = if (args.length() == 2) {
        args[1];
    }
    else {
        ".";
    };

    report: SiteReport = new();
    for (path: Str in STD.FS.walk(root)) {
        if (path.endsWith(".md")) {
            report.AddMarkdown(path);
        }
    }

    report.Print();
    return 0;
}
