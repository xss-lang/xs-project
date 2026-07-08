// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Sync system:

//{
The synchronization system provides:

- Mutex
- RwLock
- Arc
- Weak
- WeakControl
- Atomic values
- Async tasks
- Built-in async thread pool
- Thread channels

Synchronization misuse may produce SyncException.

SyncException may be caught normally.

Writing `throws SyncException` explicitly is not required.

Statically provable synchronization violations are compile-time errors.
Violations that can only be detected at runtime throw SyncException.
}//


// ============================================================
// Mutex
// ============================================================

imports Mutex;


// mutex creation

fn MutexCreation() {
    counter: Mutex<int32> = Mutex.new(0);
}


// locking

fn MutexLock() {
    counter: Mutex<int32> = Mutex.new(0);

    value: Mutex<int32> = counter.lock();

    *value += 1;
}

// lock():
//
// - Waits until the mutex can be acquired.
// - Does not return None.
// - Does not provide tryLock().
// - Returns Mutex<T>.
// - The returned Mutex<T> acts as the lock guard.
// - The guard provides mutable access through *guard.
// - The lock is released automatically when the guard
//   leaves its scope.


// automatic unlocking

fn MutexAutomaticUnlock() {
    counter: Mutex<int32> = Mutex.new(0);

    {
        value: Mutex<int32> = counter.lock();
        *value += 1;
    }

    // The lock has been released.

    secondValue: Mutex<int32> = counter.lock();
    *secondValue += 1;
}


// same-thread guard movement

fn MoveMutexGuard() {
    counter: Mutex<int32> = Mutex.new(0);

    firstGuard: Mutex<int32> = counter.lock();
    secondGuard: Mutex<int32> = firstGuard;

    *secondGuard += 1;
}

// After the move, firstGuard can no longer be used.
//
// A Mutex guard may move inside the same thread.
// A Mutex guard may not move to another thread.


// non-reentrant mutex

fn InvalidRecursiveMutexLock() {
    counter: Mutex<int32> = Mutex.new(0);

    firstGuard: Mutex<int32> = counter.lock();

    secondGuard: Mutex<int32> = counter.lock();
}

// The second lock() is a compile-time error because the
// same thread already holds the mutex.
//
// Mutex<T> is not reentrant.


// poisoning

// If a thread terminates with an error while holding a mutex,
// the mutex becomes poisoned.
//
// A later lock() operation throws SyncException.


// poisoned mutex recovery

fn RecoverPoisonedMutex() {
    counter: Mutex<int32> = Mutex.new(0);

    try {
        value: Mutex<int32> = counter.lock();
    }
    catch (error: SyncException) {
        counter.unlock();
    }
}

// unlock():
//
// - Primarily exists to recover a poisoned mutex.
// - May also manually unlock a normal mutex.
// - Manual unlocking during normal use is discouraged.
// - Automatic scope-based unlocking should be preferred.
// - Calling unlock() from a thread that does not own the
//   lock throws SyncException.
// - Calling unlock() when the lock is already open throws
//   SyncException.


// fairness

// Mutex uses a fair FIFO waiting queue.
//
// The thread that begins waiting first receives the lock first.
// Waiting threads are not starved.


// mutex trait rules

//{
// Mutex<T>: Send if T: Send
// Mutex<T>: Sync if T: Send
//}}


// mutex movement between threads

imports Thread;

fn MoveMutexToThread() {
    counter: Mutex<int32> = Mutex.new(42);

    Thread.spawn(move fn() {
        value: Mutex<int32> = counter.lock();
        *value += 1;
    });

    // counter can no longer be used here.
}


// ============================================================
// RwLock
// ============================================================

imports RwLock;


// rwlock creation

fn RwLockCreation() {
    value: RwLock<Int> = RwLock.new(42);
}


// read lock

fn RwLockRead() {
    value: RwLock<Int> = RwLock.new(42);

    reader: RwLock<Int> = value.read();

    result: Int = *reader;
}

// read():
//
// - Waits until a read lock can be acquired.
// - Returns RwLock<T>.
// - The returned value acts as a reader guard.
// - The reader provides immutable access.
// - The reader lock is released automatically when the
//   guard leaves its scope.


// invalid reader mutation

fn InvalidReaderMutation() {
    value: RwLock<Int> = RwLock.new(42);

    reader: RwLock<Int> = value.read();

    *reader = 50;
}

// Assigning through a reader is a compile-time error.


// write lock

fn RwLockWrite() {
    value: RwLock<Int> = RwLock.new(42);

    writer: RwLock<Int> = value.write();

    *writer += 1;
}

// write():
//
// - Waits until an exclusive write lock can be acquired.
// - Returns RwLock<T>.
// - The returned value acts as a writer guard.
// - The writer provides mutable access.
// - The writer lock is released automatically when the
//   guard leaves its scope.


// multiple readers

fn MultipleReaders() {
    value: RwLock<Int> = RwLock.new(42);

    reader1: RwLock<Int> = value.read();
    reader2: RwLock<Int> = value.read();

    first: Int = *reader1;
    second: Int = *reader2;
}

// Multiple readers may hold the lock simultaneously.


// writer exclusivity

// While a writer is active:
//
// - New readers wait.
// - New writers wait.
//
// A writer begins only after all active readers have left.


// reader and writer guard movement

fn MoveRwLockGuards() {
    value: RwLock<Int> = RwLock.new(42);

    reader: RwLock<Int> = value.read();
    otherReader: RwLock<Int> = reader;

    // reader can no longer be used.
}

// Reader and writer guards:
//
// - May move within the same thread.
// - May not move to another thread.


// non-reentrant rwlock

fn InvalidReadToWriteUpgrade() {
    value: RwLock<Int> = RwLock.new(42);

    reader: RwLock<Int> = value.read();
    writer: RwLock<Int> = value.write();
}

// Acquiring write() while the same thread holds read()
// is a compile-time error when statically visible.


// invalid writer reentry

fn InvalidWriterReentry() {
    value: RwLock<Int> = RwLock.new(42);

    writer: RwLock<Int> = value.write();

    reader: RwLock<Int> = value.read();
    secondWriter: RwLock<Int> = value.write();
}

// Both operations are invalid.
//
// RwLock<T> is not reentrant.
//
// Dynamically detected reentry violations throw SyncException.


// rwlock poisoning

// If a thread terminates with an error while holding a writer
// lock, the RwLock becomes poisoned.
//
// Later read() or write() calls throw SyncException.


// poisoned rwlock recovery

fn RecoverPoisonedRwLock() {
    value: RwLock<Int> = RwLock.new(42);

    try {
        writer: RwLock<Int> = value.write();
    }
    catch (error: SyncException) {
        value.rwunlock();
    }
}

// rwunlock():
//
// - Primarily exists to recover a poisoned RwLock.
// - May manually unlock a normal RwLock.
// - Manual use during normal operation is discouraged.
// - Calling rwunlock() from a thread that does not own the
//   relevant lock throws SyncException.
// - Calling rwunlock() when no relevant lock is held throws
//   SyncException.


// rwlock fairness

// RwLock uses one shared fair FIFO queue for readers and writers.
//
// Consecutive readers at the front of the queue may run together.
//
// Example:
//
// R1, R2, W1, R3
//
// Execution:
//
// R1 and R2 run together.
// W1 runs after both readers leave.
// R3 runs after W1 leaves.
//
// Example:
//
// R1, R2, R3, W1
//
// Execution:
//
// R1, R2 and R3 run together.
// W1 runs after all three readers leave.
//
// Example:
//
// R1, W1, R2, R3
//
// Execution:
//
// R1 runs.
// W1 runs after R1 leaves.
// R2 and R3 run together after W1 leaves.


// rwlock trait rules

//{
// RwLock<T>: Send if T: Send
// RwLock<T>: Sync if T: Send + Sync
//}}


// ============================================================
// Arc
// ============================================================

imports Arc;


// arc creation

fn ArcCreation() {
    value: Arc<Str> = Arc.new("Alfa");
}


// arc cloning

fn ArcCloning() {
    value: Arc<Str> = Arc.new("Alfa");

    second: Arc<Str> = Arc.clone(&value);
    third: Arc<Str> = Arc.clone(&value);
}

// Arc.clone(&value):
//
// - Borrows value immutably.
// - Does not move value.
// - Atomically increments the strong reference counter.
// - May be called repeatedly while value remains alive.


// arc access

fn ArcRead() {
    strong: Arc<Int> = Arc.new(42);

    value: Int = *strong;
}

// Dereferencing Arc<T> provides immutable access.


// invalid arc mutation

fn InvalidArcMutation() {
    strong: Arc<Int> = Arc.new(42);

    *strong = 50;
}

// Direct mutation through Arc<T> is a compile-time error.


// arc destruction

// When the final strong Arc<T> owner is destroyed:
//
// - The strong counter reaches zero.
// - The contained T value is destroyed automatically.
//
// Arc uses atomic reference counting.


// arc trait rules

//{
// Arc<T>: Send if T: Send + Sync
// Arc<T>: Sync if T: Send + Sync
//}}


// shared mutex

fn SharedMutex() {
    shared: Arc<Mutex<Str>> =
        Arc.new(Mutex.new("Alfa"));

    second: Arc<Mutex<Str>> =
        Arc.clone(&shared);

    guard: Mutex<Str> = shared.lock();

    *guard = "Beta";
}


// shared rwlock

fn SharedRwLock() {
    shared: Arc<RwLock<Int>> =
        Arc.new(RwLock.new(42));

    second: Arc<RwLock<Int>> =
        Arc.clone(&shared);

    reader: RwLock<Int> = shared.read();
}


// ============================================================
// Weak and WeakControl
// ============================================================

imports WeakControl;


// weak creation

fn WeakCreation() {
    strong: Arc<Str> = Arc.new("Alfa");

    weak: Weak<Str> = Arc.downgrade(&strong);
}

// Arc.downgrade(&strong):
//
// - Creates a Weak<T>.
// - Atomically increments the weak reference counter.
// - Does not increase the strong reference counter.
// - Does not extend the lifetime of the contained value.


// weak cloning

fn WeakCloning() {
    strong: Arc<Str> = Arc.new("Alfa");
    weak: Weak<Str> = Arc.downgrade(&strong);

    otherWeak: Weak<Str> =
        WeakControl.clone(&weak);
}

// WeakControl.clone(&weak):
//
// - Atomically increments the same weak reference counter
//   used by Arc.downgrade().
// - Does not increment the strong reference counter.


// weak upgrade

fn WeakUpgrade() {
    strong: Arc<Str> = Arc.new("Alfa");
    weak: Weak<Str> = Arc.downgrade(&strong);

    strongAgain: Arc<Str> =
        WeakControl.upgrade(&weak);
}

// WeakControl.upgrade(&weak):
//
// - Atomically increments the strong counter when successful.
// - Returns Arc<T>.
// - Throws SyncException if the contained value has already
//   been destroyed.


// weak destruction

// Weak<T> does not keep the contained value alive.
//
// When the strong count reaches zero:
//
// - The contained value is destroyed.
//
// When the final Weak<T> is also destroyed:
//
// - The shared control block is released.


// weak trait rules

//{
// Weak<T>: Send if T: Send + Sync
// Weak<T>: Sync if T: Send + Sync
//}}


// ============================================================
// Atomic
// ============================================================

imports Atomic;


// atomic creation

fn AtomicCreation() {
    Atomic counter: Int = Atomic.new(42);
    Atomic flag: Bool = Atomic.new(false);
}

// Atomic.new() requires a compile-time constant initial value.


// supported atomic value types

//{
// int16
// int32
// Int
// int128
//
// Uint16
// Uint32
// Uint
// Uint128
//
// Bool
//}}


// atomic ordering values

//{
// Atomic.ordering.Relaxed
// Atomic.ordering.Acquire
// Atomic.ordering.Release
// Atomic.ordering.AcqRel
// Atomic.ordering.SeqCst
//}}


// atomic load

fn AtomicLoad() {
    Atomic counter: Int = Atomic.new(42);

    value: Int =
        counter.load(Atomic.ordering.SeqCst);
}


// atomic store

fn AtomicStore() {
    Atomic counter: Int = Atomic.new(42);

    counter.store(
        43,
        Atomic.ordering.SeqCst
    );
}


// atomic exchange

fn AtomicExchange() {
    Atomic counter: Int = Atomic.new(42);

    oldValue: Int =
        counter.exchange(
            50,
            Atomic.ordering.SeqCst
        );
}

// exchange() returns the previous value.


// atomic addition

fn AtomicFetchAdd() {
    Atomic counter: Int = Atomic.new(42);

    oldValue: Int =
        counter.fetchAdd(
            1,
            Atomic.ordering.SeqCst
        );
}

// fetchAdd() returns the previous value.


// atomic subtraction

fn AtomicFetchSub() {
    Atomic counter: Int = Atomic.new(42);

    oldValue: Int =
        counter.fetchSub(
            1,
            Atomic.ordering.SeqCst
        );
}

// fetchSub() returns the previous value.


// compare and exchange

fn AtomicCompareExchange() {
    Atomic counter: Int = Atomic.new(42);

    oldValue: Int =
        counter.compareExchange(
            42,
            43,
            Atomic.ordering.SeqCst,
            Atomic.ordering.SeqCst
        );
}

// compareExchange():
//
// - First argument: expected value.
// - Second argument: replacement value.
// - Third argument: success ordering.
// - Fourth argument: failure ordering.
// - Returns the previous value.


// Bool atomic operations

fn AtomicBool() {
    Atomic flag: Bool = Atomic.new(false);

    current: Bool =
        flag.load(Atomic.ordering.SeqCst);

    flag.store(
        true,
        Atomic.ordering.SeqCst
    );

    previous: Bool =
        flag.exchange(
            false,
            Atomic.ordering.SeqCst
        );

    oldValue: Bool =
        flag.compareExchange(
            false,
            true,
            Atomic.ordering.SeqCst,
            Atomic.ordering.SeqCst
        );
}

// Atomic<Bool> supports:
//
// - load
// - store
// - exchange
// - compareExchange
//
// Atomic<Bool> does not support:
//
// - fetchAdd
// - fetchSub


// invalid direct atomic read

fn InvalidDirectAtomicRead() {
    Atomic counter: Int = Atomic.new(42);

    value: Int = counter;
}

// Direct atomic reads are compile-time errors.
// load() is required.


// invalid direct atomic assignment

fn InvalidDirectAtomicAssignment() {
    Atomic counter: Int = Atomic.new(42);

    counter = 50;
}

// Direct assignment is a compile-time error.
// Atomic mutation must use an atomic operation.


// invalid atomic initial value

fn InvalidRuntimeAtomicInitialization() {
    value: Int = GetRuntimeValue();

    Atomic counter: Int = Atomic.new(value);
}

// Atomic.new() requires a compile-time constant.


// atomic movement

fn MoveAtomic() {
    Atomic first: Int = Atomic.new(42);

    Atomic second: Int = first;

    // first can no longer be used.
}

// Atomic values are move-only.
//
// Sharing an atomic value between multiple threads requires Arc.


// shared atomic

fn SharedAtomic() {
    shared: Arc<Atomic<Int>> =
        Arc.new(Atomic.new(0));
}


// ordering validation

fn InvalidLoadOrdering() {
    Atomic counter: Int = Atomic.new(42);

    value: Int =
        counter.load(Atomic.ordering.Release);
}

// Release is invalid for load().
// This is a compile-time error.


fn InvalidStoreOrdering() {
    Atomic counter: Int = Atomic.new(42);

    counter.store(
        50,
        Atomic.ordering.Acquire
    );
}

// Acquire is invalid for store().
// This is a compile-time error.


// compareExchange failure ordering

// Release and AcqRel may not be used as the failure ordering
// of compareExchange().
//
// Invalid ordering combinations are compile-time errors.


// unsupported atomic operations

// Atomic does not provide:
//
// - fetchAnd
// - fetchOr
// - fetchXor
// - Other bitwise fetch operations


// ============================================================
// Async tasks
// ============================================================


// async result

async fn GetValue() => Task<Int> {
    return 42;
}


// async unit result

async fn RunUnitTask() => Task<()> {
    return;
}

// Task<()> may use return;.
//
// Returning a value from Task<()> is a compile-time error.


// awaiting

async fn AwaitValue() => Task<()> {
    value: Int = await GetValue();
}

// await:
//
// - May only appear inside async fn.
// - Begins execution of a lazy task.
// - Suspends the current task until completion.
// - Produces the T value from Task<T>.


// lazy execution

async fn LazyTask() => Task<()> {
    task: Task<Int> = GetValue();

    // GetValue has not started yet.

    value: Int = await task;

    // GetValue begins when first awaited.
}


// mandatory await

async fn InvalidUnawaitedTask() => Task<()> {
    task: Task<Int> = GetValue();
}

// Every created Task<T> must be awaited at least once.
// Leaving a task unawaited is a compile-time error.


// repeated sequential await

async fn RepeatedAwait() => Task<()> {
    task: Task<Int> = GetValue();

    first: Int = await task;
    second: Int = await task;
}

// The same Task<T> may be awaited multiple times sequentially.
// Completed tasks preserve their result.


// concurrent await

// The same Task<T> may not be actively awaited by multiple
// async flows at the same time.
//
// A statically visible concurrent await is a compile-time error.


// task movement

async fn MoveTask() => Task<()> {
    firstTask: Task<Int> = GetValue();
    secondTask: Task<Int> = firstTask;

    value: Int = await secondTask;
}

// Task<T> is move-only.
// It cannot be copied.
//
// After movement, firstTask can no longer be used.


// task thread rule

//{
// Task<T>: Send if T: Send
// Task<T> is not Sync
//}}

// A Task<T> may move to another thread when T: Send.
//
// A Task<T> is not shared concurrently between threads.


// async exception handling

async fn AsyncExceptionHandling() => Task<()> {
    try {
        value: Int = await FailingTask();
    }
    catch (error: IOException) {
    }
}

// Exceptions raised by an awaited task are transferred to the
// await expression.
//
// A possible exception must be caught at the await point.
//
// Leaving a possible async exception unhandled is a
// compile-time error.


// async runtime

//{
// The async executor is built into the language.
//
// Async tasks run on a built-in thread pool.
//
// The thread pool size is selected automatically.
//
// The user does not manually initialize or configure the
// executor.
//}}


// ============================================================
// Thread channels
// ============================================================

imports Thread, Stdio;


// channel creation and communication

fn ChannelExample() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    Thread.spawn(move fn() {
        tx.send(42);
    });

    value: Int = rx.recv();

    std.cout << value << "\n";
}

// Thread.channel<T>:
//
// - Creates one sender endpoint and one receiver endpoint.
// - Uses a single-sender, single-receiver model.
// - Sender and receiver endpoints are move-only.
// - Endpoints cannot be cloned.


// sending

fn ChannelSendMovesValue() {
    (tx, rx): Thread.channel<Str> =
        Thread.channel();

    message: Str = "Alfa";

    tx.send(message);

    // message can no longer be used.
}

// send(value) transfers ownership of value into the channel.


// receiving

fn ChannelReceive() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    Thread.spawn(move fn() {
        tx.send(42);
    });

    value: Int = rx.recv();
}

// recv():
//
// - Blocks until a value becomes available.
// - Returns T.
// - Throws SyncException if the channel is closed.


// receiver movement

fn MoveReceiverToThread() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    Thread.spawn(move fn() {
        value: Int = rx.recv();
    });

    tx.send(42);
}

// rx may move to another thread.


// invalid sender cloning

fn InvalidSenderClone() {
    (tx, rx): Thread.channel<Int> =
        Thread.channel();

    secondSender: Thread.channel<Int> =
        Thread.channel.clone(&tx);
}

// Channel endpoints cannot be cloned.
// This is a compile-time error.


// ============================================================
// SyncException
// ============================================================

fn CatchSyncException() {
    mutex: Mutex<Int> = Mutex.new(42);

    try {
        value: Mutex<Int> = mutex.lock();
    }
    catch (error: SyncException) {
    }
}

// SyncException covers runtime synchronization failures,
// including:
//
// - Poisoned Mutex access
// - Poisoned RwLock access
// - Invalid runtime lock ownership
// - Invalid runtime unlocking
// - Runtime-detected lock reentry
// - Failed Weak upgrade
// - Receiving from a closed channel
//
// Statically provable misuse is rejected during compilation.


// ============================================================
// Shared-memory summary
// ============================================================

//{
// T
//     Single ownership and ordinary single-thread use.
//
// Arc<T>
//     Multi-thread shared immutable access.
//
// Arc<Mutex<T>>
//     Multi-thread shared mutable access with one active
//     lock holder.
//
// Arc<RwLock<T>>
//     Multi-thread shared access optimized for many readers
//     and fewer writers.
//
// Arc<Atomic<T>>
//     Multi-thread shared lock-free atomic value access.
//}//
