# Phase - 1 (Planning and Stuffs)

- As of now I have a basic flow of the application. A C++ Database server which stores thing in-memory.
- Then we will have a CLI layer
- Then we will have a network layer which will let users interact with the server
- At last a JS SDK (and maybe in future python SDK as well) which acts as API layer that lets the application actually utilise the C++ database

# Phase - 2 (Basic DB)

- BTW read about ODR (One definition rule), declare every thing in one file with '.h' and use that everywehre we want to actually use the stuff from that file and then only compile that files .cpp once, else if we include the cpp file directly evrywhere, it will make the same files copy everywhere and resulting in erro because of multiple definitions of the same functions/variables etc.
- I have made Status.h, which is an enum class containing common errors, then a Response.h, which is a struct, which acts as a data type that has at max three things, status, message and data, so we can reuse it to send any response from any of our application
- I have also made Database.h, which has all the declarations and Database.cpp, which has the implementation of the Database, which is using a hashmap and returns Response for everything
- Modified the Response.h to use `std::variant` instead of `std::string`, which will let us store different types of data in the same Response struct
- Also we have now added more functions for string and also added for lists. Now every method related to setting and pushing has ttl property which is added to a custom DBEntry data type.
- We lazily check on all these operation if our key has expired by checking/verifying with the current time and if it does, we simply delete the key and never return anything
- Also for setting we are simply adding the current time to the ttl time set by the user
- Also we use somthing called std::holds_alternative, which is a function which helps in checking what kind of data is currently stored in our `std::variant`, it returns a boolean value, and after confirmation we use std::get to extract the that data
- I have setup CMake, so now just go to build folder and run `cmake ..` then `make`, it will build the project and create an executable file named `rampage_cli` which is our CLI application, we can also run `db_test` for test purposes
- Now at this point we have basic functions/methods with us that store data in the unordered_map of key as string and val as either string or deque. We also have TTL support now, with lazy deletion as of now (have to make some auto-deletion after few second using multithreading later)

# Phase - 3 (rampage_cli)

- Now we want to interact with the DB thorough our CLI and for that we have to register some commands and based on the command, we can do the necessary thing
- Ran command: `./rampage_cli`
Viewed main.cpp:9-27

Here is a clear breakdown of how the architecture we've built flows from end to end!

### 1. `CommandRegistry` (The Dispatcher)
Think of the Command Registry as a telephone switchboard operator. 
- **`CommandRegistry.h`** defines a hash map (`std::unordered_map`) where the keys are command names (like `"SET"`, `"LPUSH"`) and the values are C++ functions (`std::function`) that know how to execute that command.
- **`CommandRegistry.cpp`** does the heavy lifting when you type into the CLI. When you type `set one 3`, the `execute()` function kicks in. It calls `tokenize` to split the string into `["set", "one", "3"]`. It takes the first word (`"set"`), capitalizes it to `"SET"`, looks it up in the hash map, and hands the rest of the arguments (`["one", "3"]`) over to the specific function registered for `"SET"`.

### 2. `StringCommands.cpp` & `ListCommands.cpp` (The Modules)
If we put every single command inside `main.cpp`, it would quickly turn into a 10,000-line unreadable file filled with `if-else` statements. 

To solve this, we modularized the commands. 
- These files act as "plugins." They each expose a single setup function (like `registerStringCommands`). 
- Inside these setup functions, we define **lambdas** (anonymous, inline C++ functions) and map them to their string names (e.g., `reg.registerCommand("SET", [](...) { ... })`).
- These lambdas act as the "bridge" between the CLI and the Database. They take the raw string arrays from the CLI, do safety checks (like "did the user provide enough arguments?"), convert strings to integers if necessary, call the actual `Database` method, and then format the `Response` struct into the final string that gets printed to the screen.

### 3. `rampage_cli.cpp` (The Glue)
This is where everything is wired together into a running application.
1. **Setup:** It creates exactly one instance of the `Database` and exactly one instance of the `CommandRegistry`.
2. **Registration:** It calls `registerStringCommands(registry)` and `registerListCommands(registry)`. This passes the empty registry into our modules, where it gets fully "loaded up" with all the lambda functions.
3. **The Loop:** It starts the infinite `while (std::getline(...))` loop, constantly waiting for your input.
4. **Execution:** When you press Enter, it hands the raw string you typed directly to `registry.execute(db, line)`. The registry finds the right lambda, passes the `db` into it so the database can be modified, and spits out a resulting string.
5. **Output:** `rampage_cli.cpp` passes that result through `prettyPrint` and prints it to your terminal!

This architecture makes it incredibly easy to add new features. If you wanted to add Hash maps tomorrow, you wouldn't have to touch `rampage_cli.cpp`'s logic at all—you'd just make a `HashCommands.cpp` file, register it, and it would instantly work!

## Phase - 4 (TCP Server)

- I am trying to have a similar architecture to redis, that is my db server will allow multiple clients to connect to it at the same time, all of them can access the data present in the db.
- But instead of spawning threads per client, I am using a single main thread with epoll, which is a Linux-specific API for efficient I/O multiplexing (One thread/process watches many connections at a time), it allows a single thread to monitor multiple file descriptors (like sockets) at the same time and only react when one of them is actually ready to read/write, which is way more efficient than spawning a thread per client
- We have Server.cpp, Server.h and main.cpp for the entire server setup. Server.h declares everything, Server.cpp has the actual implementation, and main.cpp is the entry point which sets up the db, registers all commands and starts the server
- We have a `start()` method in Server.cpp which sets up the socket (bind, listen), creates the epoll instance, and then enters the event loop using `epoll_wait()` which blocks until any client (or the server socket itself) has data ready
- When epoll says the server socket is ready, it means a new client wants to connect, so we `accept()` it, set it to non-blocking mode and register it with epoll too. We also create a personal buffer (just an empty string for now) for that client in a map
- When epoll says a client socket is ready, it means that client has sent some data, so we `recv()` it and append it to that client's personal buffer
- We also have a `processClientBuffer()` method which processes all the complete commands (lines ending with `\n`) sitting in the client's buffer, one by one sequentially. TCP is a stream protocol so data might arrive in chunks, so keeping a per-client buffer and only executing when we see a `\n` is the right approach
- Since everything runs in a single thread, there is no concept of race conditions at all!
- Improve error handling. Now for any error/non-success, the redis server sends a ERR:... and for success SUCC:...., so by proper parsing the start of the strings, we can differentiate between success and error, and for success case we have further different return types based on the command.

# Phase - 5 (Nodejs SDK)

- Implement an ESM based SDK
- It is made to exaclty feel like the Redis SDK
- Users can call methods like client.get('name)
- RampageClient formats strings
- It is added to the RampageConnection class and request is added to a queue and command is added to TCP socket
- Data comming from the server is in chunks and connection maintiains a string buffer and appends incoming chunks till it sees a \n
- After a \n is received, then that command is resolved and popoed from the queue
- Other error handling like number handling, no key handling, ERR, SUCC handling is done
- We have Auto reconnect, exponential backoff, event emitter (connect, error, close, reconnecting), in-flight draining in RampageConnection Class
- Implementing a custom Rampage Error class for handling specific errors

# Phase - 6 (Persistence)

- When user sends any command, it is first executed, then only if it is related to write/update/del and only after the command is successull, we are adding the exact command in our buffer, also for ttl we are putting the final expiray time using epoch_time in the buffer

- Due to this the main-thread is never blocked, all we have to do is just log the command in buffer (in-memory)

- Meanwhile a background thread, take commands out of the queue and writes them to a file as it is

- When the db server is restarted, we keep on accpeting client's connection but first replay the entire log files commands one-by-one to repopulate the db, also if expiry time is less than current time, we don't log at all, else we log with the exact final expiry time. Also this is a blocking code, this is to avoid race coniditions/stale data. So users command will not be executed until and unless the entire repopulation is done. 

- As of now the log files keep growing indefinitely and also might contain usless commands it in as well, for example if we have `set foo one` and later `set foo two`, then the first command is useless and is taking our space in disk, but we will optimise it later and not now. Also expired data's command will still be present in the log file, which will again be optimsie in the future.

## Producer - Consumer Problem (Context: `src/persistence/PersistenceManager.h` / `.cpp`)

The relevant files here are `PersistenceManager.h` (class declaration), `PersistenceManager.cpp` (implementation), and `CommandRegistry.cpp` (where `logCommand` is actually called from).

### The Setup

After `server.start()` is called in `main.cpp`, we have exactly two threads running at the same time:

1. **Main epoll thread** (the producer) — this is the same single thread that runs the entire event loop. When a client sends a write command like `SET foo bar`, the epoll thread executes it, and then calls `CommandRegistry::logIfWriteCommand()` → which calls `PersistenceManager::logCommand()` → which pushes the command string into `queue_` (a `std::queue<std::string>` member variable inside `PersistenceManager`).

2. **Background flusher thread** (the consumer) — this is spawned in the `PersistenceManager` constructor: `flusherThread_(&PersistenceManager::flusherLoop, this)`. It runs `flusherLoop()` in a loop forever, waking up whenever `queue_` has something in it, draining it, and writing it to `rampage.rampage`.

Both threads share the **same single `queue_`** object — it's a member variable of the one `PersistenceManager` instance that lives in `main.cpp`. Both threads have access to `this`, so they both see and touch the exact same memory. This is the classic producer-consumer setup.

---

### Why this is a Problem

`std::queue` is not thread-safe. Internally, it's a wrapper around a `std::deque`, which stores its data across multiple memory chunks with a bunch of internal bookkeeping pointers ("where does the front start", "where does the back end", "how many elements are there", etc.).

When you call `queue_.push(entry)`, that is NOT a single CPU instruction. It's several steps: allocate space, write the data, update the back pointer, increment the size counter. If the main thread is **halfway through** those steps — say, it's written the data but hasn't updated the size counter yet — and the flusher thread wakes up at that exact nanosecond and calls `std::swap(localQueue, queue_)`, it is going to read an internally inconsistent queue. The item might appear to not exist, or the internal pointers might be pointing into garbage. This is undefined behaviour in C++ — meaning it might crash, it might silently corrupt data, or it might work fine 10,000 times and blow up on the 10,001st depending on how the OS scheduled the threads that particular run.

This is the **race condition** — the outcome of the program depends on which thread "wins the race" to the memory at any given nanosecond, which is something you have absolutely zero control over.

---

### Problem 1: Race Condition on `queue_`

**Where it happens:** Inside `PersistenceManager::logCommand()` (main thread pushing) and `PersistenceManager::flusherLoop()` (flusher thread swapping/reading) — both touch the same `queue_` member variable simultaneously.

**Fix: `std::mutex mutex_` + `std::lock_guard`**

A mutex (mutual exclusion lock) is a primitive that guarantees only one thread can be inside a protected section at a time. If thread A holds the lock, thread B hitting the same lock just blocks — it pauses and waits — until A releases it. No racing possible.

In `logCommand()`:
```cpp
void PersistenceManager::logCommand(const std::string& entry) {
    {
        std::lock_guard<std::mutex> lock(mutex_);  // acquire lock
        queue_.push(entry);                        // touch queue_ safely
    }                                              // lock_guard destructor releases lock here
    cv_.notify_one();
}
```

In `flusherLoop()`:
```cpp
std::unique_lock<std::mutex> lock(mutex_);
cv_.wait(lock, [this] { return !queue_.empty() || shutdown_.load(); });
std::swap(localQueue, queue_);   // touch queue_ safely — we hold the lock
// lock released when unique_lock goes out of scope
```

`std::lock_guard` is a RAII wrapper — the moment the `{ }` block ends, it automatically releases the lock even if an exception is thrown. You never have to manually call `unlock()` and risk forgetting it.

Now no matter what the OS scheduler does, the two threads can never be inside those `{ }` blocks at the same time. Race condition eliminated.

---

### Problem 2: Busy-Waiting (CPU Spinning)

Even after fixing the race condition with a mutex, there's a second problem. Most of the time, the `queue_` is empty — the flusher has nothing to do. The naive approach would be:

```cpp
// BAD: busy-waiting
while (true) {
    lock(mutex_);
    if (!queue_.empty()) { /* drain it */ }
    unlock(mutex_);
    // loop immediately and check again
}
```

This is called **busy-waiting** — the flusher thread is just spinning in a loop, constantly locking, checking, unlocking, and repeating millions of times per second even when there is literally nothing to do. It doesn't corrupt data, but it pegs an entire CPU core at 100% usage for no reason. On a multi-core machine you'd be wasting one full core just doing nothing useful.

**Fix: `std::condition_variable cv_`**

A condition variable lets a thread go genuinely to sleep — consuming zero CPU — and only wake up when another thread explicitly signals it. Think of it as a doorbell. Instead of the flusher standing at the counter checking every millisecond, it sits down and waits. The main thread rings the bell (`cv_.notify_one()`) the instant it pushes something, and only then does the flusher wake up.

In `flusherLoop()`:
```cpp
std::unique_lock<std::mutex> lock(mutex_);
cv_.wait(lock, [this] { return !queue_.empty() || shutdown_.load(); });
```

`cv_.wait()` does three things atomically:
1. Releases the mutex (so the main thread can push to `queue_` while the flusher is sleeping)
2. Puts the flusher thread fully to sleep (zero CPU)
3. When `cv_.notify_one()` is called from `logCommand()`, it re-acquires the mutex and wakes up

The lambda `[this] { return !queue_.empty() || shutdown_.load(); }` is a guard against **spurious wakeups** — a known quirk of condition variables where the OS can occasionally wake a thread up for no reason. The lambda makes the thread check the actual condition and go back to sleep if it was a false alarm.

In `logCommand()`:
```cpp
cv_.notify_one();  // ring the doorbell — wake up the flusher
```

This is called AFTER releasing the mutex (after the `{ }` block), so the flusher can immediately acquire the lock when it wakes up.

---

### Problem 3: Holding the Lock During File I/O (Blocking the Main Thread)

Even with the mutex and condition variable in place, there's a third subtle problem. If the flusher thread held the mutex for the entire duration of writing to disk — which can take microseconds to milliseconds — then every time the main thread tried to call `logCommand()`, it would block and wait for the disk write to finish. That defeats the whole point of having a background thread.

**Fix: `localQueue` — drain first, write later**

The trick is to separate "touching the shared queue" (which must be under lock) from "writing to disk" (which doesn't need the lock at all, since it's purely local):

```cpp
// In flusherLoop():
std::queue<std::string> localQueue;   // local to this thread, nobody else sees it
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [...]);
    std::swap(localQueue, queue_);    // steal everything from shared queue in one O(1) swap
}                                     // lock released here — main thread unblocked immediately

// Now write to disk with ZERO lock held
while (!localQueue.empty()) {
    file << localQueue.front() << "\n";
    localQueue.pop();
}
file.flush();
```

`std::swap` on two `std::queue` objects is an O(1) pointer swap — it just exchanges the internal pointers of the two queues, not the actual data. So the lock is held for a nanosecond (just the swap), then released immediately. The main thread is unblocked the instant the swap is done. The actual slow file I/O happens entirely outside the lock, using `localQueue` which is a purely local variable that only the flusher thread can see.

Result: the main thread is almost never blocked. The flusher does all its slow work privately. This is why the design is non-blocking.

---

### Summary of all three problems and fixes

| Problem | Where it happens | Fix | In our code |
|---|---|---|---|
| Race condition on `queue_` | `logCommand()` + `flusherLoop()` both touching `queue_` | `std::mutex mutex_` + `std::lock_guard` / `std::unique_lock` | `PersistenceManager.h` declares `mutex_`, both functions lock before touching `queue_` |
| Busy-waiting (CPU spinning) | Flusher checking queue in a loop when it's empty | `std::condition_variable cv_` — flusher sleeps, main rings bell | `cv_.wait()` in `flusherLoop()`, `cv_.notify_one()` in `logCommand()` |
| Holding lock during disk I/O | Would block main thread for every write | `localQueue` — swap under lock, write outside lock | `std::swap(localQueue, queue_)` inside `{ }`, `file <<` outside `{ }` in `flusherLoop()` |

The final result: the main epoll thread calls `logCommand()`, holds the mutex for ~1 nanosecond to push to the queue, notifies the flusher, and immediately continues handling the next client event. The flusher wakes up, steals the queue contents in one swap, releases the lock, and writes to disk on its own time. Both threads are fully independent after the swap.