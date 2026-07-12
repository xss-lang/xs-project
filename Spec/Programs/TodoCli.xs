// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small command-line todo database that persists tasks to a text file.

module Programs.TodoCli;

imports Stdio, FS, Collections, Process;

enum data TodoError {
    Io: IOException,
    InvalidCommand: Str,
    InvalidId: Int,
}

enum data Command {
    Add: Str,
    Done: Int,
    List,
    Help,
}

data TodoItem {
    id: Int;
    title: Str;
    done: Bool;
}

class TodoStore {
    path: Str;
    nextId: Int;
    items: STD.Collections.vector<TodoItem>;

    TodoStore(path: Str) throws TodoError {
        this.path = path;
        this.nextId = 1;
        this.items = STD.Collections.vector<TodoItem>.new();
        this.Load();
    }

    fn Add(title: Str) throws TodoError {
        item: TodoItem = TodoItem {
            id: this.nextId,
            title: title,
            done: false,
        };

        this.nextId += 1;
        this.items.push(item);
        this.Save();
    }

    fn MarkDone(id: Int) throws TodoError {
        for (index: Int = 0; index < this.items.length(); index = index + 1) {
            if (this.items[index].id == id) {
                this.items[index].done = true;
                this.Save();
                return;
            }
        }

        throw TodoError.InvalidId(id);
    }

    fn Print() throws IOException {
        if (this.items.length() == 0) {
            println!("No tasks yet.");
            return;
        }

        for (item: TodoItem in this.items) {
            status: Str = if (item.done) {
                "x";
            }
            else {
                " ";
            };
            println!("[{}] #{} {}", status, item.id, item.title);
        }
    }

    fn Load() throws TodoError {
        if (!STD.FS.exists(this.path)) {
            return;
        }

        content: Str = STD.FS.readToStr(this.path);

        for (line: Str in content.lines()) {
            if (line.length() == 0) {
                continue;
            }

            item: TodoItem = TodoCodec.Parse(line);
            this.items.push(item);

            if (item.id >= this.nextId) {
                this.nextId = item.id + 1;
            }
        }
    }

    fn Save() throws TodoError {
        if (!STD.FS.exists(this.path)) {
            STD.FS.createFile(this.path);
        }

        opened: STD.FS.File = STD.FS.OpenOptions.new()
            .truncate(true)
            .open(this.path);

        for (item: TodoItem in this.items) {
            STD.FS.write(opened, format!("{}\n", TodoCodec.Format(item)));
        }
    }
}

class TodoCodec {
    static fn Parse(line: Str) => TodoItem throws TodoError {
        parts: STD.Collections.vector<Str> = line.split("|");
        if (parts.length() != 3) {
            throw TodoError.InvalidCommand(line);
        }

        return TodoItem {
            id: Int.Parse(parts[0]),
            title: parts[2],
            done: parts[1] == "done",
        };
    }

    static fn Format(item: TodoItem) => Str {
        state: Str = if (item.done) {
            "done";
        }
        else {
            "open";
        };
        return item.id.ToString() + "|" + state + "|" + item.title;
    }
}

fn ParseCommand(args: STD.Collections.vector<Str>) => Command throws TodoError {
    if (args.length() < 2) {
        return Command.Help;
    }

    return match (args[1]) {
        "add" -> {
            if (args.length() < 3) {
                throw TodoError.InvalidCommand("todo add <title>");
            }
            Command.Add(args.slice(2).join(" "));
        },
        "done" -> {
            if (args.length() != 3) {
                throw TodoError.InvalidCommand("todo done <id>");
            }
            Command.Done(Int.Parse(args[2]));
        },
        "list" -> {
            Command.List;
        },
        "help" -> {
            Command.Help;
        },
        else -> {
            throw TodoError.InvalidCommand(args[1]);
        },
    };
}

fn PrintHelp() throws IOException {
    println!("todo add <title>");
    println!("todo done <id>");
    println!("todo list");
}

fn Main(args: STD.Collections.vector<Str>) => Int throws TodoError, IOException {
    store: TodoStore = new("todo.db");
    command: Command = ParseCommand(args);

    match (command) {
        Command.Add(title) -> {
            store.Add(title);
        },
        Command.Done(id) -> {
            store.MarkDone(id);
        },
        Command.List -> {
            store.Print();
        },
        Command.Help -> {
            PrintHelp();
        },
    }

    return 0;
}
