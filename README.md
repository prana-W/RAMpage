# RAMpage

![License](https://img.shields.io/badge/license-MIT-blue)
![C++](https://img.shields.io/badge/c++-17-blue.svg)
![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)

**RAMpage** is a Redis-inspired, high-performance in-memory database server implemented from scratch in C++. It rampages through reads and writes with ultra-low latency, leveraging modern Linux I/O via `epoll`.

> **Note**: Because the application uses `epoll`, which is a Linux native library, the server must be run in a Linux environment.

It comes packaged with a professional-grade **JavaScript SDK** (`rampage-js`) so you can integrate it directly into your Node.js backend seamlessly.

---

## ⚡ Core Features

### The C++ Database Server
- **Ultra-Fast I/O Engine**: Built using Linux `epoll` for non-blocking, event-driven networking. Handles thousands of concurrent persistent TCP connections on a single thread with zero race conditions.
- **Robust Custom Protocol**: A streamlined text protocol where every response is either `SUCC:<payload>` or `ERR:<message>`.
- **String & List Operations**: Full support for Redis-like primitives (`SET`, `GET`, `DEL`, `LPUSH`, `RPOP`, `LRANGE`, etc.).
- **TTL & Expiry**: Native support for key expiration (`EXPIRE`, `TTL`) automatically managed by the database.
- **Interactive CLI**: Comes with a `rampage-cli` tool to interactively run commands against the server.

### The JavaScript SDK (`rampage-js`)
- **Native ESM**: Pure Node.js ES modules, ready for modern applications with zero external dependencies.
- **Promise-Based**: `async/await` ready API (`const user = await client.get('user')`).
- **Persistent Connection Management**: Maintains a single persistent TCP connection. Commands are queued securely and responses matched in-order.
- **Auto-Reconnect**: Exponential backoff reconnect logic handles server restarts seamlessly.
- **Rich Parsing**: Automatically converts server strings into native JavaScript Types (numbers, string arrays, `null` for missing keys).
- **Strong Errors**: Throws parsed `RampageError` objects with machine-readable codes (e.g. `KEY_NOT_FOUND`, `WRONG_TYPE`).

---

## 🛠️ Commands Supported

**Strings**:
- `SET <key> <value> [ttl_ms]`
- `GET <key>`
- `DEL <key>`
- `TTL <key>`
- `EXPIRE <key> <ttl_ms>`
- `APPEND <key> <value>`
- `STRLEN <key>`

**Lists**:
- `LPUSH <key> <value> [ttl_ms]`
- `RPUSH <key> <value> [ttl_ms]`
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

### 2. Connect via CLI

In a separate terminal, you can interactively run commands against your Docker instance using the built-in CLI:
```bash
docker exec -it rampage-instance /app/rampage_cli
```
```text
rampage-cli> set name "Alice"
OK
rampage-cli> get name
"Alice"
rampage-cli> rpush tasks "Email Users"
(integer) 1
```

### 3. Use the JavaScript SDK

*(Note: I will later publish the JS-SDK to npm, so that the code can be easily installed via `npm install rampage-js`)*

Add the SDK to your Node.js project:

```js
import { createClient } from './sdk/rampage-js/src/index.js'; // Soon: import { createClient } from 'rampage-js'

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
- **`sdk/rampage-js/`** — The Node.js client package.
- **`tests/`** — Server tests and Node.js testing playground.
- **`docs/`** — Internal design notes.
