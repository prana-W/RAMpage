# rampage-js

> Official JavaScript SDK for [RAMpage](https://github.com/prana-W/RAMpage) — a high-performance in-memory database with a Redis-like command surface.

[![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)](https://nodejs.org)
[![ESM](https://img.shields.io/badge/module-ESM-blue)](https://nodejs.org/api/esm.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Features

- 🔌 **Persistent TCP connection** — one connection, zero overhead per command
- ⚡ **Async/await API** — every method returns a Promise
- 🔄 **Auto-reconnect** — exponential backoff, configurable retries
- 🎯 **Typed responses** — numbers, arrays, and strings (not raw buffers)
- 🚨 **Rich error handling** — `RampageError` with machine-readable `code` fields
- 📦 **Zero dependencies** — uses only Node.js built-ins (`net`, `events`)
- 🟢 **ESM only** — modern ES modules

---

## Installation

```bash
npm install rampage-js
```

> Requires **Node.js ≥ 18** and a running RAMpage server.

---

## Quick Start

```js
import { createClient } from 'rampage-js';

// 1. Create a client
const client = createClient({
  host: '127.0.0.1', // default
  port: 2006,        // default
});

// 2. Connect
await client.connect();

// 3. Use it
await client.set('name', 'Alice');
const name = await client.get('name'); // 'Alice'

// 4. Disconnect when done
await client.disconnect();
```

---

## API Reference

### `createClient(options?)`

Factory function. Returns a `RampageClient` instance.

```js
import { createClient } from 'rampage-js';

const client = createClient({
  host: '127.0.0.1',   // RAMpage server hostname/IP   (default: '127.0.0.1')
  port: 2006,           // RAMpage server port           (default: 2006)
  autoReconnect: true,  // Auto-reconnect on disconnect  (default: true)
  maxRetries: 5,        // Max reconnect attempts        (default: 5, -1 = infinite)
  retryDelay: 500,      // Base delay ms (exponential)   (default: 500)
  connectTimeout: 5000, // Initial connect timeout ms    (default: 5000)
});
```

---

### Lifecycle

```js
await client.connect();     // Establish TCP connection — must be called first
await client.disconnect();  // Gracefully close the connection
```

#### Events

```js
client.on('connect',      ()           => console.log('Ready!'));
client.on('error',        (err)        => console.error(err.message));
client.on('close',        ()           => console.log('Disconnected'));
client.on('reconnecting', ({ attempt, delay }) =>
  console.log(`Reconnect attempt ${attempt} in ${delay}ms`)
);
```

---

### String Commands

#### `client.set(key, value, options?)`
Sets a key to a string value.

| Param | Type | Description |
|---|---|---|
| `key` | `string` | Key name |
| `value` | `string \| number` | Value to store |
| `options.ttl` | `number` *(optional)* | Time-to-live in **seconds** |

```js
await client.set('name', 'Alice');
await client.set('token', 'abc123', { ttl: 60 }); // expires in 60s
```

---

#### `client.get(key)` → `string | null`
Gets the value of a key. Returns `null` if the key doesn't exist.

```js
const val = await client.get('name'); // 'Alice'
const mis = await client.get('nope'); // null
```

---

#### `client.del(key)` → `string`
Deletes a key.

```js
await client.del('name');
```

---

#### `client.ttl(key)` → `number`
Returns the remaining TTL of a key in seconds.

| Return value | Meaning |
|---|---|
| `N` | Seconds remaining |
| `-1` | Key exists, no expiry |
| `-2` | Key does not exist |

```js
const secs = await client.ttl('token'); // e.g. 57
```

---

#### `client.expire(key, seconds)` → `string`
Sets or refreshes the TTL on an existing key.

```js
await client.expire('token', 120);
```

---

#### `client.append(key, value)` → `number`
Appends to a string value. Returns the new string length.

```js
const len = await client.append('log', ' new entry'); // 10
```

---

#### `client.strlen(key)` → `number`
Returns the length of the string value at a key.

```js
const len = await client.strlen('name'); // 5
```

---

### List Commands

#### `client.lpush(key, value, options?)` → `number`
Pushes a value to the **head** (left) of a list. Returns new list length.

```js
await client.lpush('tasks', 'task-1');
await client.lpush('tasks', 'task-0', { ttl: 300 });
```

---

#### `client.rpush(key, value, options?)` → `number`
Pushes a value to the **tail** (right) of a list. Returns new list length.

```js
await client.rpush('tasks', 'task-2');
```

---

#### `client.lpop(key)` → `string | null`
Removes and returns the **head** (left) element. Returns `null` if empty.

```js
const task = await client.lpop('tasks'); // 'task-0'
```

---

#### `client.rpop(key)` → `string | null`
Removes and returns the **tail** (right) element. Returns `null` if empty.

```js
const task = await client.rpop('tasks'); // 'task-2'
```

---

#### `client.llen(key)` → `number`
Returns the number of elements in a list.

```js
const n = await client.llen('tasks'); // 3
```

---

#### `client.lindex(key, index)` → `string | null`
Returns the element at `index`. Negative indices count from the tail.

```js
const first = await client.lindex('tasks', 0);   // 'task-0'
const last  = await client.lindex('tasks', -1);  // 'task-2'
```

---

#### `client.lset(key, index, value)` → `string`
Overwrites the element at `index`.

```js
await client.lset('tasks', 0, 'urgent-task');
```

---

#### `client.lrange(key, start, stop)` → `string[]`
Returns a JS **array** of elements from `start` to `stop` (inclusive). Negative indices count from the tail.

```js
const all   = await client.lrange('tasks', 0, -1);  // ['task-0', 'task-1', 'task-2']
const first = await client.lrange('tasks', 0, 0);   // ['task-0']
```

---

### Escape Hatch

#### `client.sendCommand(rawCommand)` → `string`
Send any raw command string directly to RAMpage. Returns the raw response string — no parsing, no error-throwing. Useful for commands not yet in the SDK.

```js
const resp = await client.sendCommand('SET foo bar');
console.log(resp); // 'Key set successfully'

const val = await client.sendCommand('GET foo');
console.log(val); // 'bar'
```

---

## Error Handling

All SDK methods throw a `RampageError` (extends `Error`) when the server returns an error. Always wrap commands in `try/catch` or chain `.catch()`.

```js
import { createClient, RampageError } from 'rampage-js';

try {
  await client.get('wrong-type-key'); // key holds a list, not a string
} catch (err) {
  if (err instanceof RampageError) {
    console.error(err.message); // 'Wrong type operation'
    console.error(err.code);    // 'WRONG_TYPE'
  }
}
```

### Error Codes

| Code | Meaning |
|---|---|
| `KEY_NOT_FOUND` | Key does not exist |
| `WRONG_TYPE` | Command used on a key with a different data type |
| `WRONG_ARGS` | Wrong number of arguments passed |
| `UNKNOWN_COMMAND` | Command not recognised by RAMpage |
| `OUT_OF_BOUNDS` | List index is out of range |
| `CONNECTION_ERROR` | TCP socket error or not connected |
| `CONNECTION_TIMEOUT` | Could not connect within the configured timeout |
| `MAX_RETRIES_EXCEEDED` | Auto-reconnect exhausted all retries |
| `PARSE_ERROR` | Unexpected response format (SDK bug — please report) |
| `ERR` | Generic error |

> **Important:** A `RampageError` only affects your Node.js application. The RAMpage C++ server is completely unaffected by errors thrown in client code.

---

## Full Example

```js
import { createClient, RampageError } from 'rampage-js';

const client = createClient({ port: 2006, maxRetries: -1 });

client.on('connect',      ()  => console.log('✅ Connected'));
client.on('reconnecting', ({ attempt }) => console.log(`🔄 Reconnect #${attempt}`));
client.on('error',        (e) => console.error('❌', e.message));

await client.connect();

// Strings
await client.set('user:1:name', 'Alice');
await client.set('user:1:token', 'xyz', { ttl: 3600 });
const name = await client.get('user:1:name');

// Lists
await client.rpush('jobs', 'send-email');
await client.rpush('jobs', 'resize-image');
const jobs = await client.lrange('jobs', 0, -1); // ['send-email', 'resize-image']
const next = await client.lpop('jobs');           // 'send-email'

// Error handling
try {
  await client.get('jobs'); // 'jobs' is a list!
} catch (err) {
  console.error(`[${err.code}] ${err.message}`); // [WRONG_TYPE] Wrong type operation
}

await client.disconnect();
```

Run the full example:

```bash
cd sdk/rampage-js
node examples/basic.js
```

---

## Protocol Notes

RAMpage uses a dead-simple raw TCP text protocol:
- **Request:** `COMMAND arg1 arg2\n`
- **Response:** one line of text ending in `\n`
- Values with spaces must be quoted: `SET key "hello world"`
- Commands are case-insensitive (`SET`, `set`, `Set` all work)
- Default port: **2006**
