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

# Phase - 7: REdis Serialization Protocol (RESP) Layer

## What is RESP and Why Did We Add It?
RESP (REdis Serialization Protocol) is the networking protocol used by Redis to communicate between clients and the server. It defines how data (strings, integers, arrays, errors) should be formatted when sent over a TCP socket.

**Why we added it:**
Before this phase, RAMpage used a naive newline-separated protocol (`GET foo\n`). While simple to build, it meant we couldn't use standard Redis tools. By implementing RESP, RAMpage is now a "Redis-compatible" server. We can:
- Use `redis-cli` to interact with our database natively.
- Use `redis-benchmark` to test our server's performance under heavy load.
- Use any standard Redis client library (in Node.js, Python, Go, etc.) to connect to RAMpage.

## The Architecture Change

**Old Architecture:**
```
TCP bytes → newline-split → tokenize (space/quotes) → CommandRegistry::execute() → "SUCC:/ERR:" string → TCP send
```

**New Architecture:**
```
TCP bytes → RESPParser (handles pipelining/partial frames) → vector<string> tokens → CommandRegistry::execute() → RESPSerializer → TCP send
```
*Crucially, our internal command handlers (`Database.cpp`, `StringCommands.cpp`, etc.) were **not changed**. They still return internal strings like `"SUCC:1"` or `"SUCC:PONG"`. The RESP layer acts as an adapter, translating incoming RESP bytes to tokens, and outgoing internal strings to RESP bytes.*

## How We Implemented It (File by File)

### 1. `src/protocol/RESPParser.h & .cpp`
This is a stateless parser that takes the raw incoming byte buffer from a client and extracts a command.
- **Array Parsing:** It understands standard RESP arrays like `*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n`.
- **Inline Fallback:** Tools like `telnet` and sometimes `redis-cli` (for simple commands like `PING`) send "inline" commands without the `*` or `$`. The parser detects this and falls back to space-separated tokenization.
- **Partial Frame Buffering:** If `recv()` only gives us half a packet (e.g., `*3\r\n$3\r\nSE`), the parser returns `INCOMPLETE`. The server leaves the buffer alone and waits for the next `recv()` to complete it.

### 2. `src/protocol/RESPSerializer.h & .cpp`
This class translates our internal `SUCC:/ERR:` formats into RESP wire types.
- **Response Mapping:** It looks at the command name to know how to format the data.
  - `SUCC:` (empty) for a `GET` means the key is missing → `$-1\r\n` (Null Bulk String).
  - `SUCC:` (empty) for a `SET` means success → `+OK\r\n` (Simple String).
  - `SUCC:1` for `DEL` means an integer return → `:1\r\n` (Integer).
  - `SUCC:a|b` for `LRANGE` means an array return → `*2\r\n$1\r\na\r\n$1\r\nb\r\n` (Array).
  - `ERR:message` → `-ERR message\r\n` (Error).

### 3. `src/server/Server.cpp` (The Pipeline Loop)
We completely rewrote `processClientBuffer`.
- **Greedy Pipelining:** Tools like `redis-benchmark` send hundreds of commands in a single TCP write. Our new `Server` uses a `while(true)` loop to greedily parse and execute as many complete RESP commands as possible from the client's buffer before returning to `epoll_wait`.

### 4. `src/commands/CommandRegistry.cpp`
- Added an overloaded `execute(Database& db, std::vector<std::string>& tokens)` method. Since the `RESPParser` already tokenizes the incoming command, we bypass the old `tokenize()` step entirely for network clients, saving CPU cycles.
- Modified the AOF logging logic for `DEL` to only log if the key was *actually* deleted (i.e., when `DEL` returns `SUCC:1`).

### 5. `src/commands/*Commands.cpp`
We updated the command handlers slightly to match Redis semantics perfectly:
- Added a `PING` command (returns `SUCC:PONG`), which is the first thing `redis-cli` and `redis-benchmark` send to check if the server is alive.
- Modified `DEL` and `EXPIRE` to return `SUCC:1` or `SUCC:0` (integers) instead of void success, matching Redis.
- Modified `GET`, `LPOP`, `RPOP`, and `LINDEX` to return `SUCC:` (which maps to a Null Bulk String) on a cache miss, instead of throwing an `ERR:` message.

### Summary
By cleanly separating the networking protocol (RESP) from the database logic, we made RAMpage highly compatible with industry-standard tools without rewriting the core engine!

## Phase - 7.1: Updating the Node.js SDK for RESP

Because the RAMpage server now speaks RESP, our custom Node.js SDK (`rampage-js`) broke. We took the opportunity to rename it to follow better conventions and rewrote it to speak proper RESP.

**1. Renamed to `rampage-node`**
We renamed the directory from `sdk/rampage-js` to `sdk/rampage-node` and updated the `package.json` name to match.

**2. `src/connection.js` (The core parser update)**
- Replaced the old `\n`-based line buffer with a new, robust `RespReader` class.
- The `RespReader` accumulates raw TCP chunks and statefully parses RESP frames (Simple Strings `+`, Errors `-`, Integers `:`, Bulk Strings `$`, and Arrays `*`).
- In `sendCommand(args)`, we no longer send a space-separated string. We encode the command as a RESP Array frame (`*N\r\n$len\r\narg\r\n...`) and send that directly to the server.

**3. `src/parser.js` (Thin type-assertion layer)**
- We deleted all the old logic that was parsing the internal `SUCC:/ERR:` strings.
- Now, it simply receives the fully decoded JS value (e.g. `1`, `'OK'`, `['a', 'b']`) from the `RespReader` and acts as a type-assertion layer (e.g. `parseInteger` ensures it actually got a number).

**4. `src/client.js` (Command definitions)**
- We removed the `quoteIfNeeded` function entirely! RESP natively handles multi-word arguments because every string explicitly declares its byte length.
- Every method (e.g. `set`, `get`) was updated to pass an array of arguments (e.g. `['SET', 'key', 'value']`) into `_send()` instead of a single string.
- The `DEL` and `EXPIRE` methods were updated to expect numbers (`1` or `0`) instead of plain strings, matching the new Redis-compatible backend behavior.

The `rampage-node` SDK is now a fully-featured RESP client perfectly aligned with how real Redis drivers work!

# Phase - 8 (Micro-Optimisations)

Before:

| Command | Redis (req/s) | RAMpage (req/s) | Redis p50 (ms) | RAMpage p50 (ms) | Verdict |
|---|---|---|---|---|---|
| PING_INLINE | 82,237 | 93,721 | 0.471 | 0.287 | RAMpage **faster** |
| PING_MBULK | 81,301 | 94,607 | 0.463 | 0.207 | RAMpage **faster** |
| SET | 76,805 | 79,302 | 0.559 | 0.407 | RAMpage **faster** |
| GET | 80,128 | 54,615 | 0.511 | 0.119 | Throughput: Redis wins. Latency: RAMpage wins |
| LPUSH | 80,451 | 68,399 | 0.551 | 0.247 | Throughput: Redis wins. Latency: RAMpage wins |
| RPUSH | 81,367 | 79,872 | 0.543 | 0.127 | Roughly tied throughput, RAMpage **lower latency** |
| LPOP | 80,192 | 80,515 | 0.551 | 0.415 | RAMpage **faster** |
| RPOP | 80,064 | 80,257 | 0.559 | 0.423 | RAMpage **faster** |
| LRANGE_100 | 59,137 | 6,288 | 0.479 | 7.671 | Redis **~9.4x faster** |
| LRANGE_300 | 31,888 | 2,494 | 0.783 | 18.655 | Redis **~12.8x faster** |
| LRANGE_500 | 22,232 | 1,667 | 1.119 | 28.975 | Redis **~13.3x faster** |
| LRANGE_600 | 19,505 | 1,273 | 1.271 | 37.759 | Redis **~15.3x faster** |


After:

| Command | Redis (req/s) | RAMpage (req/s) | Throughput Verdict | Redis p50 (ms) | RAMpage p50 (ms) | Latency Verdict |
|---|---|---|---|---|---|---|
| PING_INLINE | 93,633 | 89,047 | Redis 1.05x faster | 0.407 | 0.295 | RAMpage 1.4x faster |
| PING_MBULK | 100,806 | 86,207 | Redis 1.2x faster | 0.391 | 0.223 | RAMpage 1.8x faster |
| SET | 97,561 | 81,833 | Redis 1.2x faster | 0.455 | 0.255 | RAMpage 1.8x faster |
| GET | 99,305 | 82,305 | Redis 1.2x faster | 0.415 | 0.335 | RAMpage 1.2x faster |
| LPUSH | 97,371 | 82,305 | Redis 1.2x faster | 0.447 | 0.095 | RAMpage 4.7x faster |
| RPUSH | 98,425 | 54,377 | Redis 1.8x faster | 0.447 | 0.103 | RAMpage 4.3x faster |
| LPOP | 96,993 | 81,235 | Redis 1.2x faster | 0.455 | 0.095 | RAMpage 4.8x faster |
| RPOP | 94,429 | 82,305 | Redis 1.1x faster | 0.463 | 0.127 | RAMpage 3.6x faster |
| LRANGE_100 | 67,659 | 21,906 | Redis 3.1x faster | 0.391 | 2.151 | Redis 5.5x faster |
| LRANGE_300 | 38,820 | 8,678 | Redis 4.5x faster | 0.647 | 5.455 | Redis 8.4x faster |
| LRANGE_500 | 26,062 | 5,564 | Redis 4.7x faster | 0.959 | 8.711 | Redis 9.1x faster |
| LRANGE_600 | 21,730 | 4,645 | Redis 4.7x faster | 1.119 | 10.279 | Redis 9.2x faster |

## What Was the Problem?

Our LRANGE command was **~15x slower than Redis**. Every other command (SET, GET, LPUSH, RPOP, etc.) was at par or even faster than Redis, but LRANGE with 600 elements took 37ms vs Redis's 1.2ms. The question was: why?

## Root Cause Analysis

We traced the full LRANGE code path and found **5 compounding bottlenecks** — the data was being copied, joined, split, and rebuilt multiple times before finally being sent over the wire.

### The Old Data Flow (before optimization)

```
1. Database::lrange()
   → Copies 600 strings from deque into a new vector
   → Copies the entire vector into the Response variant

2. ListCommands.cpp LRANGE handler
   → Joins all 600 strings with '|' using O(n²) string concatenation (payload += "|" + vec[i])
   → Returns "SUCC:a|b|c|d|e|..."

3. RESPSerializer::serialize()
   → Calls splitPipe() which creates a std::istringstream, re-parses the pipe string back into a NEW vector of 600 strings
   → Calls array() which does ANOTHER O(n²) loop: out += bulkString(item)

4. Server::processClientBuffer()
   → Calls send() individually for EVERY command in the pipeline (one syscall per command)
```

**The pipe-join → split → rejoin dance was the core problem.** We were joining 600 strings into one pipe-separated string, only to immediately split it back into 600 strings and re-encode them as RESP. This is like translating English → French → English → German when you could have just gone English → German directly.

## What We Changed (File by File)

### 1. `Database.cpp` — `std::string_view` zero-copy lrange result
Changed the `lrange` method to return a `std::vector<std::string_view>` instead of `std::vector<std::string>`.
Previously, the line `res.push_back(deq[i])` was forcing a **deep copy** of every single string inside the database's deque. For an `LRANGE_600`, this meant allocating 600 brand new strings on the heap *every single time* the command was called!
By returning a `string_view` (which is just a non-owning pointer and a length), we completely eliminate all string allocations inside the database. The `ListCommands` handler now reads directly from the database's internal memory when building the RESP output.

### 2. `ListCommands.cpp` — Direct RESP serialization for LRANGE
This was the biggest change. Instead of pipe-joining all elements and returning `SUCC:a|b|c`, the LRANGE handler now builds the RESP array bytes directly:

```cpp
std::string resp;
resp.reserve(totalSize);  // single allocation!
resp += "*600\r\n";
for (item : vec) resp += "$3\r\nfoo\r\n";
return "RESP:" + resp;
```

The `RESP:` prefix is a signal to the server: "these bytes are already in RESP wire format, send them directly without going through the serializer." This completely eliminates the pipe-join → splitPipe → array rebuild dance.

### 3. `RESPSerializer.cpp` — Two fixes
- **`array()` method**: Added `reserve()` to pre-calculate the total output size and do a single memory allocation. Before, the loop `out += bulkString(item)` was O(n²) because each `+=` potentially reallocated the entire string.
- **`serialize()` method**: Replaced `result.substr(0, 4) == "ERR:"` with `result.compare(0, 4, "ERR:") == 0`. `substr()` creates a temporary string (heap allocation), while `compare()` does an in-place comparison with zero allocations.

### 4. `Server.cpp` — Two fixes
- **RESP: prefix detection**: If the command handler returns a string starting with `RESP:`, the server appends the bytes (from offset 5) directly to the output buffer, skipping the serializer entirely.
- **Batched send()**: Instead of calling `send()` after every single command in a pipeline, we now accumulate all responses in a single `outBuf` string and do ONE `send()` syscall at the end of the pipeline loop. For `redis-benchmark` which pipelines 16 commands per batch, this reduces syscalls from 16 to 1 per batch.

### 5. `RESPParser.cpp` — Replaced `std::istringstream`
In the inline command parser (used by `PING_INLINE`), replaced the `std::istringstream` tokenizer with a manual `string::find(' ')` loop. `istringstream` constructs a heavyweight stream object on every call — the manual split is a simple pointer scan with zero overhead.

## Why These Changes Work

| Optimization | What it eliminates |
|---|---|
| `std::string_view` in lrange | Deep copy of 600 string elements (600 allocations) |
| Direct RESP build in handler | The entire pipe-join → split → rebuild pipeline |
| `reserve()` in array() | O(n²) reallocation → O(n) single allocation |
| `compare()` vs `substr()` | Temporary string allocation on every command |
| Batched `send()` | 16 syscalls → 1 syscall per pipeline batch |
| Manual split vs istringstream | Stream object construction/destruction per command |

## Extreme Micro-Optimizations (Phase 8b)

Even after zero-copying the vectors, C++ was still hiding major runtime overhead compared to C (Redis). We added 4 aggressive optimizations to completely eliminate all remaining hidden C++ allocations:

1. **Eliminated `std::to_string` Overhead**: `std::to_string` internally allocates a temporary string on the heap for every single number. We replaced it with C++17's `<charconv>` (`std::to_chars`), writing sizes directly into our buffer pointer without allocations.
2. **Eliminated `+` Operator Allocation**: `return "RESP:" + resp` forced C++ to allocate a completely new 10KB string and copy both sides into it. We instead `.reserve(totalSize + 5)` and `resp.append("RESP:*")` directly to the buffer, eliminating the massive allocation.
3. **Eliminated Output Buffer Reallocations**: `redis-benchmark` pipelines up to 16 commands at once, producing ~160KB of data for `LRANGE_600`. `std::string outBuf` in `Server.cpp` was forced to reallocate its internal buffer over 10 times per pipeline batch. We added `outBuf.reserve(128 * 1024)` at the top of the function to eliminate this.
4. **Eliminated `std::deque` Index Arithmetic**: We changed the `for` loop in `Database::lrange` from `deq[i]` to `auto it = deq.begin() + start; ++it`. Deque indexing requires modulo arithmetic on every lookup; iterators just use simple pointer increments.
5. **Global Serializer Bypass**: We went back and updated *all* other commands (`SET`, `GET`, `PUSH`, `POP`, etc.) in `StringCommands.cpp` and `ListCommands.cpp`. Instead of returning `"SUCC:"` (which forces `RESPSerializer::serialize` to allocate a temporary string and rebuild the output), every command now builds and returns the raw `RESP:` bytes directly. This gives a massive free throughput boost across the board by completely eliminating the `RESPSerializer` from the hot path for all successful commands.

# Phase - 9 (Pub/Sub)

We implemented a full Redis-compatible Pub/Sub engine. This includes exact channel subscriptions (`SUBSCRIBE`, `UNSUBSCRIBE`), pattern-based subscriptions (`PSUBSCRIBE`, `PUNSUBSCRIBE`), publishing messages (`PUBLISH`), and cluster introspection (`PUBSUB CHANNELS/NUMSUB/NUMPAT`). 

## The Core Challenge
Redis Pub/Sub fundamentally breaks the standard client-server **Request-Response** model.
In normal operation: Client sends a command -> Server processes it -> Server sends a response.
In Pub/Sub:
1. Client A sends `SUBSCRIBE` and enters a special "Subscriber Mode".
2. Client B sends `PUBLISH`.
3. The server asynchronously pushes a message to Client A, without Client A requesting it.

This means the server must maintain state about which clients are listening to what, and command handlers need the ability to "push" data directly to specific client sockets.

## What We Changed (Architecture Flow)

### 1. `PubSubManager` (The State Registry)
We created a dedicated `PubSubManager` class (`src/pubsub/PubSubManager.h`) to manage all subscription state.
- **Exact Channels**: Maps `channel_name -> set<client_fd>` and reverse `client_fd -> set<channel_name>`.
- **Pattern Channels**: Maps `glob_pattern -> set<client_fd>` and reverse.
- **Publishing**: The `publish()` method iterates through the matching sockets, formats the payload into a standard RESP push array (using `RESPSerializer::pushMessage`), and calls the OS-level `send(fd, ...)` directly to fan-out the message to all subscribers instantly.

### 2. Threading `clientFd` into Command Handlers
Previously, command handlers only knew about the `Database` and the arguments (`std::vector<std::string>& args`). They didn't know *who* they were talking to.
To let `SUBSCRIBE` register the correct client, we changed the fundamental `CommandHandler` signature in `CommandRegistry.h`:
```cpp
// Before
using CommandHandler = std::function<std::string(Database&, std::vector<std::string>&)>;

// After
using CommandHandler = std::function<std::string(Database&, int clientFd, std::vector<std::string>&)>;
```
We updated all 20+ existing commands in `StringCommands.cpp` and `ListCommands.cpp` to accept (and ignore) this new `clientFd` parameter.

### 3. Subscriber Context (The `Server` Loop)
When a client subscribes to a channel, Redis dictates they enter a special "Subscriber Context". While in this state, they are forbidden from sending normal commands (like `SET` or `GET`).
In `Server::processClientBuffer`, we added a check:
```cpp
if (pubSub_.isSubscriber(clientFd)) {
    // Only allow (P)SUBSCRIBE, (P)UNSUBSCRIBE, PING, QUIT
}
```
Furthermore, when a client disconnects, `Server::removeClient` now explicitly calls `pubSub_.unsubscribeAll(clientFd)` to clean up the socket from the registry and prevent the publisher from trying to write to dead file descriptors.

### 4. Direct RESP Formatting
Because Pub/Sub relies on pushing non-standard arrays (like `*3\r\n$9\r\nsubscribe...`), we couldn't use the standard `SUCC:` prefix. Instead, the `SUBSCRIBE` and `UNSUBSCRIBE` handlers use the `RESP:` prefix to bypass the serializer completely and send the raw byte stream back to the client.

## Why We Did It This Way
- **Zero-Blocking Fan-Out**: By calling `send()` directly inside `PubSubManager::publish()`, we fan out messages immediately on the publisher's thread cycle, avoiding the need for complex message queues or background threads. Our epoll event loop handles this seamlessly.
- **Explicit Separation of Concerns**: Keeping `PubSubManager` entirely separate from the `Database` ensures that ephemeral Pub/Sub traffic is cleanly isolated from persistent key-value data. This implicitly ensures that `PUBLISH` events never pollute the Append-Only File (AOF).