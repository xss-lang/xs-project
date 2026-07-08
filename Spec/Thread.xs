// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Thread system:

//{
The Thread module provides:

- Native thread creation
- Move-based thread closures
- Join handles
- Thread result values
- Detached threads
- Thread sleeping
- Cooperative yielding
- Single-sender, single-receiver channels

Thread synchronization failures use SyncException.

Thread values and channel endpoints follow the language's ownership,
move and Send rules.
}//

imports Thread;


// ============================================================
// Thread creation
// ============================================================


// thread without result

fn SpawnThread() {
    Thread.spawn(move fn() {
        // Thread body
    });
}


// thread with result

fn SpawnThreadWithResult() {
    thread: Thread.handle<Int> =
        Thread.spawn(move fn() => Int {
            return 42;
        });

    value: Int = thread.join();
}


// resultless thread handle

fn ResultlessThreadHandle() {
    thread: Thread.handle<()> =
        Thread.spawn(move fn() {
            return;
        });

    thread.join();
}


// Thread.spawn():
//
// - Creates a native thread.
// - Accepts a move closure.
// - Moves captured values into the new thread.
// - Returns Thread.handle<T> when the closure returns T.
// - Returns Thread.handle<()> when the closure returns no value.
// - Begins execution immediately.
// - The closure must satisfy the required Send rules.


// ============================================================
// Move captures
// ============================================================

fn MoveValueToThread() {
    value: Str = "Alfa";

    thread: Thread.handle<()> =
        Thread.spawn(move fn() {
            localValue: Str = value;
        });

    thread.join();

    // value can no longer be used here.
}


// A value captured by a move thread closure:
//
// - Is moved into the new thread.
// - Can no longer be used by the spawning thread.
// - Must satisfy Send.


// invalid non-Send capture

fn InvalidNonSendCapture() {
    value: NonSendType = NonSendType.new();

    Thread.spawn(move fn() {
        Use(value);
    });
}

// Moving a non-Send value into another thread is a compile-time error.


// ============================================================
// Thread handles
// ============================================================

fn MoveThreadHandle() {
    first: Thread.handle<Int> =
        Thread.spawn(move fn() => Int {
            return 42;
        });

    second: Thread.handle<Int> = first;

    value: Int = second.join();

    // first can no longer be used.
}


// Thread.handle<T>:
//
// - Is move-only.
// - Cannot be copied.
// - May move to another thread when T: Send.
// - Is not Sync.
// - Represents ownership of the join operation.
//
// Trait rules:
//
//{
// Thread.handle<T>: Send if T: Send
// Thread.handle<T>: not Sync
//}}


// ============================================================
// Joining
// ============================================================

fn JoinThread() {
    thread: Thread.handle<Int> =
        Thread.spawn(move fn() => Int {
            return 42;
        });

    value: Int = thread.join();
}

// join():
//
// - Blocks until the thread finishes.
// - Returns the thread result T.
// - Consumes the Thread.handle<T>.
// - May only be called once.
// - Re-throws an uncaught exception from the joined thread.
// - Throws SyncException for invalid runtime join operations.


// invalid second join

fn InvalidSecondJoin() {
    thread: Thread.handle<Int> =
        Thread.spawn(move fn() => Int {
            return 42;
        });

    value: Int = thread.join();
    secondValue: Int = thread.join();
}

// The second join() is a compile-time error because join()
// consumes the thread handle.


// self join

// A thread attempting to join itself throws SyncException.


// ============================================================
// Thread exceptions
// ============================================================

fn JoinedThreadException() {
    thread: Thread.handle<Int> =
        Thread.spawn(move fn() => Int {
            throw IOException();
        });

    try {
        value: Int = thread.join();
    }
    catch (error: IOException) {
    }
}

// If a joined thread terminates with an uncaught exception:
//
// - The exception is stored by the thread runtime.
// - join() re-throws the same exception to the joining thread.
// - The exception may be caught normally.


// detached thread exception

fn DetachedThreadException() {
    Thread.spawn(move fn() {
        throw IOException();
    });
}

// If a detached thread terminates with an uncaught exception:
//
// - Only that thread is terminated.
// - The runtime emits a diagnostic.
// - The entire process is not automatically terminated.


// ============================================================
// Detached threads
// ============================================================

fn DetachedThread() {
    Thread.spawn(move fn() {
        // Continues independently.
    });
}

// A thread is detached when its returned handle is not stored.
//
// Detached threads:
//
// - Continue running independently.
// - Cannot later be joined.
// - Do not provide their result to the spawning thread.
// - Are terminated when the process itself exits.


// ============================================================
// Sleeping and yielding
// ============================================================

fn SleepThread() {
    Thread.sleep(1000);
}

// Thread.sleep(milliseconds):
//
// - Suspends the current native thread.
// - Uses milliseconds.
// - Blocks the current thread.
// - Does not suspend only the current async task.


// cooperative yield

fn YieldThread() {
    Thread.yield();
}

// Thread.yield():
//
// - Voluntarily gives the scheduler an opportunity to run
//   another ready thread.
// - Does not guarantee that another thread will run.
// - Does not release held Mutex or RwLock guards.


// ============================================================
// Channels
// ============================================================


// basic channel

fn ChannelExample() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    Thread.spawn(move fn() {
        tx.send(42);
    });

    value: Int = rx.recv();
}


// Thread.channel<T>:
//
// - Creates one sender endpoint and one receiver endpoint.
// - Uses a single-sender, single-receiver model.
// - Sender and receiver endpoints are move-only.
// - Endpoints cannot be cloned.
// - Endpoints may move between threads when T: Send.


// sending

fn SendChannelValue() {
    (tx, rx): Thread.channel<Str> =
        Thread.channel();

    message: Str = "Alfa";

    tx.send(message);

    // message can no longer be used.
}

// tx.send(value):
//
// - Transfers ownership of value into the channel.
// - The sent value can no longer be used by the sender.
// - Throws SyncException if the channel is closed.


// receiving

fn ReceiveChannelValue() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    Thread.spawn(move fn() {
        tx.send(42);
    });

    value: Int = rx.recv();
}

// rx.recv():
//
// - Blocks until a value becomes available.
// - Returns T.
// - Transfers ownership of the received value to the receiver.
// - Throws SyncException if the channel is closed and no value
//   can be received.


// moving receiver to another thread

fn MoveReceiverToThread() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    receiverThread: Thread.handle<()> =
        Thread.spawn(move fn() {
            value: Int = rx.recv();
        });

    tx.send(42);

    receiverThread.join();
}


// moving sender to another thread

fn MoveSenderToThread() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    senderThread: Thread.handle<()> =
        Thread.spawn(move fn() {
            tx.send(42);
        });

    value: Int = rx.recv();

    senderThread.join();
}


// invalid sender clone

fn InvalidSenderClone() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    otherTx: Thread.channel<Int> =
        Thread.channel.clone(&tx);
}

// Channel endpoints cannot be cloned.
// This is a compile-time error.


// invalid receiver clone

fn InvalidReceiverClone() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    otherRx: Thread.channel<Int> =
        Thread.channel.clone(&rx);
}

// Channel endpoints cannot be cloned.
// This is a compile-time error.


// ============================================================
// Interaction with synchronization types
// ============================================================

imports Mutex, RwLock, Arc;


// moving a mutex into a thread

fn MoveMutexIntoThread() {
    mutex: Mutex<Int> = Mutex.new(42);

    thread: Thread.handle<()> =
        Thread.spawn(move fn() {
            guard: Mutex<Int> = mutex.lock();
            *guard += 1;
        });

    thread.join();
}


// sharing a mutex between threads

fn ShareMutexBetweenThreads() {
    shared: Arc<Mutex<Int>> =
        Arc.new(Mutex.new(42));

    worker: Arc<Mutex<Int>> =
        Arc.clone(&shared);

    thread: Thread.handle<()> =
        Thread.spawn(move fn() {
            guard: Mutex<Int> = worker.lock();
            *guard += 1;
        });

    thread.join();

    guard: Mutex<Int> = shared.lock();
}


// sharing an rwlock between threads

fn ShareRwLockBetweenThreads() {
    shared: Arc<RwLock<Int>> =
        Arc.new(RwLock.new(42));

    worker: Arc<RwLock<Int>> =
        Arc.clone(&shared);

    thread: Thread.handle<()> =
        Thread.spawn(move fn() {
            reader: RwLock<Int> = worker.read();
            value: Int = *reader;
        });

    thread.join();
}


// Guard values:
//
// - Mutex lock guards cannot move to another thread.
// - RwLock reader guards cannot move to another thread.
// - RwLock writer guards cannot move to another thread.
//
// Moving such a guard into Thread.spawn() is a compile-time error.


// ============================================================
// Unsupported thread operations
// ============================================================

// The Thread module does not provide:
//
// - Thread cancellation
// - Thread termination or kill
// - Thread priority control
// - CPU affinity control
// - Thread naming
// - Public thread identifier API
// - Clonable thread handles
// - Clonable channel endpoints
