// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small command-line todo database that persists tasks to a text file.

module Programs::TodoCli;

imports stdio, fs, collections, process, result;

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
    items: std::collections::vector<TodoItem>;

    TodoStore(path: Str) {
        self.path = path;
        self.nextId = 1;
        self.items = std::collections::vector<TodoItem>::new();
    }

    static fn Open(path: Str) -> Result<TodoStore, TodoError> {
        store: TodoStore = new(path);
        store.Load()@;
        return Ok(store);
    }

    fn Add(title: Str) -> Result<(), TodoError> {
        item: TodoItem = TodoItem {
            id: self.nextId,
            title: title,
            done: false,
        };

        self.nextId += 1;
        self.items.push(item);
        self.Save()@;
        return Ok();
    }

    fn MarkDone(id: Int) -> Result<(), TodoError> {
        for (index: Int = 0; index < self.items.length(); index = index + 1) {
            if (self.items[index].id == id) {
                self.items[index].done = true;
                self.Save()@;
                return Ok();
            }
        }

        return Error(TodoError::InvalidId(id));
    }

    fn Print() -> Result<(), IOException> {
        if (self.items.length() == 0) {
            println!("No tasks yet.");
            return Ok();
        }

        for (item: TodoItem in self.items) {
            status: Str = if (item.done) {
                "x";
            }
            else {
                " ";
            };
            println!("[{}] #{} {}", status, item.id, item.title);
        }
        return Ok();
    }

    fn Load() -> Result<(), TodoError> {
        if (!std::fs::exists(self.path)) {
            return Ok();
        }

        content: Str = std::fs::read_to_str(self.path);

        for (line: Str in content.lines()) {
            if (line.length() == 0) {
                continue;
            }

            item: TodoItem = TodoCodec::Parse(line)@;
            self.items.push(item);

            if (item.id >= self.nextId) {
                self.nextId = item.id + 1;
            }
        }
        return Ok();
    }

    fn Save() -> Result<(), TodoError> {
        if (!std::fs::exists(self.path)) {
            std::fs::create_file(self.path);
        }

        opened: std::fs::File = std::fs::OpenOptions::new()
            .truncate(true)
            .open(self.path);

        for (item: TodoItem in self.items) {
            std::fs::write(opened, format!("{}\n", TodoCodec::Format(item)));
        }
        return Ok();
    }
}

class TodoCodec {
    static fn Parse(line: Str) -> Result<TodoItem, TodoError> {
        parts: std::collections::vector<Str> = line.split("|");
        if (parts.length() != 3) {
            return Error(TodoError::InvalidCommand(line));
        }

        return Ok(TodoItem {
            id: Int::Parse(parts[0]),
            title: parts[2],
            done: parts[1] == "done",
        });
    }

    static fn Format(item: TodoItem) -> Str {
        state: Str = if (item.done) {
            "done";
        }
        else {
            "open";
        };
        return item.id.ToString() + "|" + state + "|" + item.title;
    }
}

fn ParseCommand(args: std::collections::vector<Str>) -> Result<Command, TodoError> {
    if (args.length() < 2) {
        return Ok(Command::Help);
    }

    return match (args[1]) {
        "add" -> {
            if (args.length() < 3) {
                return Error(TodoError::InvalidCommand("todo add <title>"));
            }
            Ok(Command::Add(args.slice(2).join(" ")));
        },
        "done" -> {
            if (args.length() != 3) {
                return Error(TodoError::InvalidCommand("todo done <id>"));
            }
            Ok(Command::Done(Int::Parse(args[2])));
        },
        "list" -> {
            Ok(Command::List);
        },
        "help" -> {
            Ok(Command::Help);
        },
        else -> {
            Error(TodoError::InvalidCommand(args[1]));
        },
    };
}

fn PrintHelp() -> Result<(), IOException> {
    println!("todo add <title>");
    println!("todo done <id>");
    println!("todo list");
    return Ok();
}

fn Main(args: std::collections::vector<Str>) -> Result<Int, Error> {
    store: TodoStore = TodoStore::Open("todo.db")@;
    command: Command = ParseCommand(args)@;

    match (command) {
        Command::Add(title) -> {
            store.Add(title)@;
        },
        Command::Done(id) -> {
            store.MarkDone(id)@;
        },
        Command::List -> {
            store.Print()@;
        },
        Command::Help -> {
            PrintHelp()@;
        },
    }

    return Ok(0);
}
