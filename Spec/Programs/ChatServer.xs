// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A tiny multi-room chat server using async sockets and channels.

module Programs::ChatServer;

imports collections, std, sync, thread, net;

enum data ChatError {
    Io: Error,
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
    members: std::collections::HashMap<ClientId, Thread.sender<Message>>;

    Room(name: Str) {
        self.name = name;
        self.members = std::collections::HashMap<ClientId, Thread.sender<Message>>::new();
    }

    fn Join(id: ClientId, sender: Thread.sender<Message>) {
        self.members[id] = sender;
    }

    fn Leave(id: ClientId) {
        self.members.remove(id);
    }

    fn Broadcast(message: Message) {
        for ((else, sender): (ClientId, Thread.sender<Message>) in self.members) {
            sender.send(message);
        }
    }
}

class ChatHub {
    nextId: Atomic<Int>;
    rooms: Arc<Mutex<std::collections::HashMap<Str, Room>>>;

    ChatHub() {
        self.nextId = Atomic::new(1);
        self.rooms = Arc::new(Mutex::new(std::collections::HashMap<Str, Room>::new()));
    }

    async fn Serve(listener: Net.tcpListener) -> Task<Result<()>> {
        while (true) {
            socket: Net.tcpStream = await listener.accept()@;
            id: ClientId = ClientId {
                value: self.nextId.fetchAdd(1),
            };
            rooms: Arc<Mutex<std::collections::HashMap<Str, Room>>> = Arc::clone(&self.rooms);

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
    rooms: Arc<Mutex<std::collections::hash_map<Str, Room>>>;

    ClientSession(id: ClientId, socket: Net.tcpStream, rooms: Arc<Mutex<std::collections::hash_map<Str, Room>>>) {
        self.id = id;
        self.socket = socket;
        self.rooms = rooms;
    }

    async fn Run() -> Task<Result<()>> {
        currentRoom: Str = "lobby";
        outbound: Thread.channel<Message> = Thread.channel<Message>();
        self.Join(currentRoom, outbound.sender());

        while (true) {
            line: Optional<Str> = await self.socket.read_line();
            command: Str = line ?? "/quit";

            if (command == "/quit") {
                self.Leave(currentRoom);
                return Ok();
            }

            if (command.startsWith("/join ")) {
                self.Leave(currentRoom);
                currentRoom = command.substring(6);
                self.Join(currentRoom, outbound.sender());
                continue;
            }

            self.Broadcast(Message {
                room: currentRoom,
                sender: self.id,
                body: command,
            });
        }
    }

    fn Join(roomName: Str, sender: Thread.sender<Message>) {
        guard: Mutex<std::collections::hash_map<Str, Room>> = self.rooms.lock();
        if (!(*guard).contains(roomName)) {
            (*guard)[roomName] = Room::new(roomName);
        }
        (*guard)[roomName].Join(self.id, sender);
    }

    fn Leave(roomName: Str) {
        guard: Mutex<std::collections::hash_map<Str, Room>> = self.rooms.lock();
        (*guard)[roomName].Leave(self.id);
    }

    fn Broadcast(message: Message) {
        guard: Mutex<std::collections::hash_map<Str, Room>> = self.rooms.lock();
        (*guard)[message.room].Broadcast(message);
    }
}

async fn Main() -> Task<Result<Int, Error>> {
    listener: Net.tcpListener = await Net.listen("127.0.0.1:9000")@;
    hub: ChatHub = new();
    await hub.Serve(listener)@;
    return Ok(0);
}
