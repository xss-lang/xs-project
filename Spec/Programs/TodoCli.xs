// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small command-line todo database that persists tasks to a text file.

module Programs.TodoCli;

imports Stdio, FS, Collections, Process, Result;

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

    TodoStore(path: Str) {
        this.path = path;
        this.nextId = 1;
        this.items = STD.Collections.vector<TodoItem>.new();
    }

    static fn Open(path: Str) => Result.Result<TodoStore, TodoError> {
        store: TodoStore = new(path);
        store.Load()@;
        return Result.Ok(store);
    }

    fn Add(title: Str) => Result.Result<Void, TodoError> {
        item: TodoItem = TodoItem {
            id: this.nextId,
            title: title,
            done: false,
        };

        this.nextId += 1;
        this.items.push(item);
        this.Save()@;
        return Result.Ok();
    }

    fn MarkDone(id: Int) => Result.Result<Void, TodoError> {
        for (index: Int = 0; index < this.items.length(); index = index + 1) {
            if (this.items[index].id == id) {
                this.items[index].done = true;
                this.Save()@;
                return Result.Ok();
            }
        }

        return Result.Error(TodoError.InvalidId(id));
    }

    fn Print() => Result.Result<Void, IOException> {
        if (this.items.length() == 0) {
            println!("No tasks yet.");
            return Result.Ok();
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
        return Result.Ok();
    }

    fn Load() => Result.Result<Void, TodoError> {
        if (!STD.FS.exists(this.path)) {
            return Result.Ok();
        }

        content: Str = STD.FS.readToStr(this.path);

        for (line: Str in content.lines()) {
            if (line.length() == 0) {
                continue;
            }

            item: TodoItem = TodoCodec.Parse(line)@;
            this.items.push(item);

            if (item.id >= this.nextId) {
                this.nextId = item.id + 1;
            }
        }
        return Result.Ok();
    }

    fn Save() => Result.Result<Void, TodoError> {
        if (!STD.FS.exists(this.path)) {
            STD.FS.createFile(this.path);
        }

        opened: STD.FS.File = STD.FS.OpenOptions.new()
            .truncate(true)
            .open(this.path);

        for (item: TodoItem in this.items) {
            STD.FS.write(opened, format!("{}\n", TodoCodec.Format(item)));
        }
        return Result.Ok();
    }
}

class TodoCodec {
    static fn Parse(line: Str) => Result.Result<TodoItem, TodoError> {
        parts: STD.Collections.vector<Str> = line.split("|");
        if (parts.length() != 3) {
            return Result.Error(TodoError.InvalidCommand(line));
        }

        return Result.Ok(TodoItem {
            id: Int.Parse(parts[0]),
            title: parts[2],
            done: parts[1] == "done",
        });
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

fn ParseCommand(args: STD.Collections.vector<Str>) => Result.Result<Command, TodoError> {
    if (args.length() < 2) {
        return Result.Ok(Command.Help);
    }

    return match (args[1]) {
        "add" -> {
            if (args.length() < 3) {
                return Result.Error(TodoError.InvalidCommand("todo add <title>"));
            }
            Result.Ok(Command.Add(args.slice(2).join(" ")));
        },
        "done" -> {
            if (args.length() != 3) {
                return Result.Error(TodoError.InvalidCommand("todo done <id>"));
            }
            Result.Ok(Command.Done(Int.Parse(args[2])));
        },
        "list" -> {
            Result.Ok(Command.List);
        },
        "help" -> {
            Result.Ok(Command.Help);
        },
        else -> {
            Result.Error(TodoError.InvalidCommand(args[1]));
        },
    };
}

fn PrintHelp() => Result.Result<Void, IOException> {
    println!("todo add <title>");
    println!("todo done <id>");
    println!("todo list");
    return Result.Ok();
}

fn Main(args: STD.Collections.vector<Str>) => Result.Result<Int, Result.Error> {
    store: TodoStore = TodoStore.Open("todo.db")@;
    command: Command = ParseCommand(args)@;

    match (command) {
        Command.Add(title) -> {
            store.Add(title)@;
        },
        Command.Done(id) -> {
            store.MarkDone(id)@;
        },
        Command.List -> {
            store.Print()@;
        },
        Command.Help -> {
            PrintHelp()@;
        },
    }

    return Result.Ok(0);
}
