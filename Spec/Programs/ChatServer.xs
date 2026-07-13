// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A tiny multi-room chat server using async sockets and channels.

module Programs.ChatServer;

imports collections, stdio, sync, thread, net, result;

enum data ChatError {
    Io: IOException,
    Protocol: Str,
    Closed,
}

data ClientId {
    value: Int;
}

data Message {
    room: Str;
    sender: ClientId;
    body: Str;
}

class Room {
    name: Str;
    members: std.collections.hash_map<ClientId, Thread.sender<Message>>;

    Room(name: Str) {
        this.name = name;
        this.members = std.collections.hash_map<ClientId, Thread.sender<Message>>.new();
    }

    fn Join(id: ClientId, sender: Thread.sender<Message>) {
        this.members[id] = sender;
    }

    fn Leave(id: ClientId) {
        this.members.remove(id);
    }

    fn Broadcast(message: Message) {
        for ((_, sender): (ClientId, Thread.sender<Message>) in this.members) {
            sender.send(message);
        }
    }
}

class ChatHub {
    nextId: Atomic<Int>;
    rooms: Arc<Mutex<std.collections.hash_map<Str, Room>>>;

    ChatHub() {
        this.nextId = Atomic.new(1);
        this.rooms = Arc.new(Mutex.new(std.collections.hash_map<Str, Room>.new()));
    }

    async fn Serve(listener: Net.tcpListener) => Task<Result.Result<Void, ChatError>> {
        while (true) {
            socket: Net.tcpStream = await listener.accept()@;
            id: ClientId = ClientId {
                value: this.nextId.fetchAdd(1),
            };
            rooms: Arc<Mutex<std.collections.hash_map<Str, Room>>> = Arc.clone(&this.rooms);

            Thread.spawn(move async fn() {
                session: ClientSession = new(id, socket, rooms);
                await session.Run()@;
            });
        }
    }
}

class ClientSession {
    id: ClientId;
    socket: Net.tcpStream;
    rooms: Arc<Mutex<std.collections.hash_map<Str, Room>>>;

    ClientSession(id: ClientId, socket: Net.tcpStream, rooms: Arc<Mutex<std.collections.hash_map<Str, Room>>>) {
        this.id = id;
        this.socket = socket;
        this.rooms = rooms;
    }

    async fn Run() => Task<Result.Result<Void, ChatError>> {
        currentRoom: Str = "lobby";
        outbound: Thread.channel<Message> = Thread.channel<Message>();
        this.Join(currentRoom, outbound.sender());

        while (true) {
            line: Optional<Str> = await this.socket.read_line();
            command: Str = line ?? "/quit";

            if (command == "/quit") {
                this.Leave(currentRoom);
                return Result.Ok();
            }

            if (command.startsWith("/join ")) {
                this.Leave(currentRoom);
                currentRoom = command.substring(6);
                this.Join(currentRoom, outbound.sender());
                continue;
            }

            this.Broadcast(Message {
                room: currentRoom,
                sender: this.id,
                body: command,
            });
        }
    }

    fn Join(roomName: Str, sender: Thread.sender<Message>) {
        guard: Mutex<std.collections.hash_map<Str, Room>> = this.rooms.lock();
        if (!(*guard).contains(roomName)) {
            (*guard)[roomName] = Room.new(roomName);
        }
        (*guard)[roomName].Join(this.id, sender);
    }

    fn Leave(roomName: Str) {
        guard: Mutex<std.collections.hash_map<Str, Room>> = this.rooms.lock();
        (*guard)[roomName].Leave(this.id);
    }

    fn Broadcast(message: Message) {
        guard: Mutex<std.collections.hash_map<Str, Room>> = this.rooms.lock();
        (*guard)[message.room].Broadcast(message);
    }
}

async fn Main() => Task<Result.Result<Int, ChatError>> {
    listener: Net.tcpListener = await Net.listen("127.0.0.1:9000")@;
    hub: ChatHub = new();
    await hub.Serve(listener)@;
    return Result.Ok(0);
}
