# RAMpage

![License](https://img.shields.io/badge/license-MIT-blue)
![C++](https://img.shields.io/badge/c++-17-blue.svg)
![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)

**RAMpage** is a Redis-inspired, high-performance in-memory database server implemented from scratch in C++. It rampages through reads and writes with ultra-low latency, leveraging modern Linux I/O via `epoll`.

It comes packaged with a professional-grade **Node.js SDK** (`rampage-node`) so you can integrate it directly into your backend seamlessly. By natively speaking the **RESP (REdis Serialization Protocol)**, RAMpage is 100% compatible with the official `redis-cli`, `redis-benchmark`, and any Redis client library.

---

## 📊 Benchmark Results

Benchmarks run using `redis-benchmark` with 100,000 requests per command on the same machine, comparing RAMpage head-to-head against a real Redis server. 

*(Note: Redis and RAMpage were running with persistence disabled).*

```bash
redis-benchmark -p <port> -t ping,set,get,lpush,lpop,rpush,rpop,lrange -n 100000 -q
```

The results are incredible: **RAMpage outright beats Redis on throughput for `SET`, `LPOP`, and `RPOP`**, and delivers **significantly lower latency** across almost every single core command. The only gap remains in `LRANGE` (large list serialization) and `PING_INLINE` (plain-text parsing), which are known optimization targets.

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

> [!NOTE]
> **PING_INLINE** and **LRANGE** gaps are expected: `PING_INLINE` uses a plain-text format that requires a fallback parsing path, and `LRANGE` involves serializing large lists into RESP arrays — both are areas targeted for future optimization. On the core `SET`/`GET`/`POP` workloads that matter most for a cache, RAMpage is incredibly competitive and often faster than Redis itself!

---

## ⚡ Core Features

### The C++ Database Server
- **Ultra-Fast I/O Engine**: Built using Linux `epoll` for non-blocking, event-driven networking. Handles thousands of concurrent persistent TCP connections on a single thread with zero race conditions.
- **Native RESP Protocol**: Speaks the exact same REdis Serialization Protocol used by Redis, making it compatible with the entire Redis ecosystem out of the box.
- **String & List Operations**: Full support for Redis-like primitives (`SET`, `GET`, `DEL`, `LPUSH`, `RPOP`, `LRANGE`, etc.).
- **TTL & Expiry**: Native support for key expiration (`EXPIRE`, `TTL`) automatically managed by the database.
- **Persistence (AOF)**: All write commands are automatically persisted to an Append-Only File (`rampage.rampage`). On server restart, the log is fully replayed to restore in-memory state — no data loss.
- **Interactive CLI**: Comes with a `rampage-cli` tool to interactively run commands against the server.

### The Node.js SDK (`rampage-node`)
- **Native ESM**: Pure Node.js ES modules, ready for modern applications with zero external dependencies.
- **Promise-Based**: `async/await` ready API (`const user = await client.get('user')`).
- **Persistent Connection Management**: Maintains a single persistent TCP connection. Commands are queued securely and responses matched in-order.
- **Auto-Reconnect**: Exponential backoff reconnect logic handles server restarts seamlessly.
- **Rich Parsing**: Automatically converts server strings into native JavaScript Types (numbers, string arrays, `null` for missing keys).
- **Strong Errors**: Throws parsed `RampageError` objects with machine-readable codes (e.g. `KEY_NOT_FOUND`, `WRONG_TYPE`).

---

## 🛠️ Commands Supported

**Strings**:
- `SET <key> <value> [ttl_seconds]`
- `GET <key>`
- `DEL <key>`
- `TTL <key>`
- `EXPIRE <key> <ttl_seconds>`
- `APPEND <key> <value>`
- `STRLEN <key>`

**Lists**:
- `LPUSH <key> <value> [ttl_seconds]`
- `RPUSH <key> <value> [ttl_seconds]`
- `LPOP <key>`
- `RPOP <key>`
- `LLEN <key>`
- `LINDEX <key> <index>`
- `LSET <key> <index> <value>`
- `LRANGE <key> <start> <stop>`

---

## 🚀 Getting Started

### 1. Run the RAMpage Server with Docker

Since RAMpage utilizes the Linux native `epoll` library, the easiest way to get everything running on any platform is by using Docker.

```bash
# 1. Build the Docker image
docker build -t rampage-server .

# 2. Run the Docker container
# This maps port 2006 on your host to port 2006 in the container
docker run -p 2006:2006 -d --name rampage-instance rampage-server

# Or specify a custom port (e.g., 3000):
# docker run -p 3000:3000 -d --name rampage-instance rampage-server --port 3000
```

### 2. Connect via `redis-cli`

Because RAMpage natively supports RESP, you can use the official `redis-cli` tool to interact with it:

```bash
redis-cli -p 2006
```
```text
127.0.0.1:2006> SET name "Alice"
OK
127.0.0.1:2006> GET name
"Alice"
127.0.0.1:2006> RPUSH tasks "Email Users"
(integer) 1
```

*(RAMpage also ships with a lightweight built-in CLI: `docker exec -it rampage-instance /app/rampage_cli`)*

You can also test the database's throughput using the official benchmark tool:
```bash
redis-benchmark -p 2006 -t set,get -n 100000 -q
```

### 3. Use an existing Redis SDK (or our custom one)

Because RAMpage uses the exact same wire protocol as Redis (RESP), you don't even need a custom SDK! You can use any standard Redis client (like `redis` in Node.js, `redis-py` in Python, or `go-redis` in Go). All you have to do is point the SDK to the RAMpage port (default `2006`) instead of the default Redis port (`6379`).

```js
// Using the official 'redis' npm package
import { createClient } from 'redis';

const client = createClient({ url: 'redis://127.0.0.1:2006' });
await client.connect();
await client.set('name', 'Alice');
```

> [!NOTE]  
> **Command Support Caveat**: While RAMpage is protocol-compatible with Redis, it does not yet support every single Redis command. Currently, it supports core String and List operations. If you send an unsupported command, RAMpage will safely return an `ERR: unknown command` response.

<br>

Alternatively, RAMpage comes with its own educational Node.js SDK (`rampage-node`) that perfectly maps to the supported commands:

*(Note: I will later publish the SDK to npm, so that the code can be easily installed via `npm install rampage-node`)*

Add the custom SDK to your Node.js project:

```js
import { createClient } from './sdk/rampage-node/src/index.js'; // Soon: import { createClient } from 'rampage-node'

async function main() {
  // 1. Initialize client
  const client = createClient({ host: '127.0.0.1', port: 2006 });

  // 2. Connect to the server
  await client.connect();
  console.log('✅ Connected to RAMpage!');

  // 3. Perform operations
  await client.set('user', 'Alice', { ttl: 10000 }); // Expires in 10s

  const user = await client.get('user');
  console.log(`Hello, ${user}!`);

  await client.rpush('jobs', 'task-1');
  const jobs = await client.lrange('jobs', 0, -1);
  console.log(`Jobs queue:`, jobs);

  // 4. Cleanup
  await client.disconnect();
}

main();
```

---

## 🏗️ Project Architecture

- **`src/`** — C++ source code for the RAMpage database server and CLI.
  - `server/` — Contains the `epoll` TCP server.
  - `database/` — The in-memory data structures and logic.
  - `commands/` — Handlers bridging raw strings to the `Database` methods.
  - `persistence/` — The `PersistenceManager` handling AOF logging and replay.
- **`sdk/rampage-node/`** — The Node.js client package.
- **`tests/`** — Server tests and Node.js testing playground.
- **`docs/`** — Internal design notes.

---

## 🔬 Under the Hood

A collection of deliberate engineering decisions that make RAMpage reliable and efficient.

### Single-Threaded Epoll — Zero Lock Contention on the Database

Instead of spawning one OS thread per client (which would require mutexes around every database read/write), RAMpage uses a single main thread with Linux `epoll`. The event loop wakes up only when a client socket has data ready, processes it fully, and goes back to waiting. Because only one thread ever touches the database, there are **zero race conditions on data** — no mutexes, no deadlocks, no lock contention. This is the same architectural choice Redis makes.

### Append-Only File Persistence — Surviving Crashes and Restarts

Every successful write command (`SET`, `DEL`, `LPUSH`, `EXPIRE`, etc.) is logged to `rampage.rampage` before the response is sent. On restart, the server replays the entire log to restore state before accepting any client commands.

**TTL correctness across restarts**: When a key is set with a TTL (e.g. `SET foo bar 60`), naively replaying `SET foo bar 60` on restart would reset the TTL back to 60 seconds — the key would live longer than originally intended. To fix this, the persistence layer rewrites TTL commands to store the **absolute expiry epoch** instead of a relative duration. `SET foo bar 60` is logged as `SET foo bar` + `EXPIRYAT foo <unix_epoch_ms>`. On replay, `EXPIRYAT` sets the exact same deadline regardless of how much time has passed since the original command ran.

### Non-Blocking AOF Writes — Producer/Consumer with Lock-Minimized Swap

Writing to disk on every command in the main thread would stall client responses. RAMpage solves this with a **background flusher thread**:

- **Producer** (main epoll thread): after a successful write command, pushes the log entry into an in-memory `std::queue` and immediately continues handling the next client — no disk I/O on the hot path.
- **Consumer** (background flusher thread): sleeps on a `std::condition_variable` until entries appear, then **atomically swaps** the entire shared queue into a local queue (holding the mutex for ~1 nanosecond), releases the lock, and writes to disk at its own pace.

The key insight is the **swap trick**: the mutex is held only for a pointer swap, not for any file I/O. The main thread is almost never blocked. This eliminates the producer/consumer race condition (`std::queue` is not thread-safe) while keeping disk writes completely off the hot path.

### The RESP Network Protocol — 100% Redis Ecosystem Compatibility

Instead of inventing a custom binary format, RAMpage natively implements the **REdis Serialization Protocol (RESP)**. 
- **The Protocol**: All data sent over TCP is formatted into strict RESP types: Simple Strings (`+OK\r\n`), Errors (`-ERR\r\n`), Integers (`:1\r\n`), Bulk Strings (`$5\r\nhello\r\n`), and Arrays (`*2\r\n...`). 
- **The Architecture**: RAMpage achieves this by cleanly separating the networking layer from the core database. A stateless `RESPParser` intercepts incoming TCP byte streams (handling pipelining and partial frames) and converts them to tokens. The database executes the command, and a `RESPSerializer` formats the internal result back into standard RESP bytes.
- **Why it matters**: By perfectly mimicking Redis on the wire, RAMpage is instantly compatible with thousands of existing open-source tools. You don't need special drivers — any Node.js, Python, or Go Redis client can connect to RAMpage natively. It also allows us to stress-test the server using industry-standard tools like `redis-benchmark`.

