import { EventEmitter } from 'events';
import { RampageConnection } from './connection.js';
import {
  parseSimple,
  parseValue,
  parseInteger,
  parseTtl,
  parseLrange,
  parseRaw,
} from './parser.js';

/**
 * Wraps a value in quotes if it contains whitespace, so the RAMpage server
 * tokenizer treats it as a single argument.
 * e.g.  "hello world" → `"hello world"`
 *       "simple"      → `simple`
 *
 * @param {string|number} val
 * @returns {string}
 */
function quoteIfNeeded(val) {
  const s = String(val);
  return /\s/.test(s) ? `"${s.replace(/"/g, '\\"')}"` : s;
}

/**
 * RampageClient
 *
 * Redis-like client for RAMpage. Every method maps to the corresponding TCP
 * command, sends it over a persistent connection, and returns a typed result.
 *
 * Usage:
 *   import { createClient } from 'rampage-js';
 *   const client = createClient({ host: '127.0.0.1', port: 2006 });
 *   await client.connect();
 *   await client.set('name', 'Alice');
 *   const name = await client.get('name'); // 'Alice'
 */
export class RampageClient extends EventEmitter {
  /**
   * @param {object} [options]
   * @param {string}  [options.host='127.0.0.1']
   * @param {number}  [options.port=2006]
   * @param {boolean} [options.autoReconnect=true]
   * @param {number}  [options.maxRetries=5]          - -1 for infinite
   * @param {number}  [options.retryDelay=500]         - base ms for exponential backoff
   * @param {number}  [options.connectTimeout=5000]    - ms to wait for initial connect
   */
  constructor(options = {}) {
    super();
    this._conn = new RampageConnection(options);

    // Forward connection events up to the client surface
    this._conn.on('connect', () => this.emit('connect'));
    this._conn.on('error', (err) => this.emit('error', err));
    this._conn.on('close', () => this.emit('close'));
    this._conn.on('reconnecting', (info) => this.emit('reconnecting', info));
  }


  /**
   * Connect to the RAMpage server. Must be awaited before issuing commands.
   * @returns {Promise<void>}
   */
  async connect() {
    await this._conn.connect();
  }

  /**
   * Disconnect from the RAMpage server.
   * @returns {Promise<void>}
   */
  async disconnect() {
    await this._conn.disconnect();
  }


  /**
   * Low-level: send a raw command string and get the raw response back.
   * @param {string} cmd
   * @returns {Promise<string>}
   */
  async _send(cmd) {
    return this._conn.sendCommand(cmd);
  }


  /**
   * SET key value [EX seconds]
   * Sets a key to a string value with an optional TTL in seconds.
   *
   * @param {string} key
   * @param {string|number} value
   * @param {object}  [options]
   * @param {number}  [options.ttl]  - Expiry in seconds
   * @returns {Promise<string>}       - 'Key set successfully'
   * @throws {RampageError}
   *
   * @example
   * await client.set('name', 'Alice');
   * await client.set('token', 'abc123', { ttl: 60 });
   */
  async set(key, value, options = {}) {
    const parts = ['SET', quoteIfNeeded(key), quoteIfNeeded(value)];
    if (options.ttl != null) parts.push(String(options.ttl));
    const raw = await this._send(parts.join(' '));
    return parseSimple(raw);
  }

  /**
   * GET key
   * Gets the string value of a key.
   *
   * @param {string} key
   * @returns {Promise<string | null>}  - The value, or null if the key stores an empty string
   * @throws {RampageError}             - KEY_NOT_FOUND if key doesn't exist, WRONG_TYPE if key holds a list
   *
   * @example
   * const val = await client.get('name'); // 'Alice'
   */
  async get(key) {
    const raw = await this._send(`GET ${quoteIfNeeded(key)}`);
    const payload = parseValue(raw);
    return payload === '' ? null : payload;
  }

  /**
   * DEL key
   * Deletes a key.
   *
   * @param {string} key
   * @returns {Promise<string>}
   * @throws {RampageError}  - KEY_NOT_FOUND if key doesn't exist
   *
   * @example
   * await client.del('name');
   */
  async del(key) {
    const raw = await this._send(`DEL ${quoteIfNeeded(key)}`);
    return parseSimple(raw);
  }

  /**
   * TTL key
   * Returns the time-to-live of a key in seconds.
   *   -1  → key exists but has no expiry
   *   -2  → key does not exist
   *    N  → seconds remaining
   *
   * @param {string} key
   * @returns {Promise<number>}
   * @throws {RampageError}
   *
   * @example
   * const secs = await client.ttl('token');
   */
  async ttl(key) {
    const raw = await this._send(`TTL ${quoteIfNeeded(key)}`);
    return parseTtl(raw);
  }

  /**
   * EXPIRE key seconds
   * Sets or updates the TTL on an existing key.
   *
   * @param {string} key
   * @param {number} seconds
   * @returns {Promise<string>}  - 'Expiry set'
   * @throws {RampageError}
   *
   * @example
   * await client.expire('token', 120);
   */
  async expire(key, seconds) {
    const raw = await this._send(`EXPIRE ${quoteIfNeeded(key)} ${seconds}`);
    return parseSimple(raw);
  }

  /**
   * APPEND key value
   * Appends a value to an existing string key (or creates it).
   *
   * @param {string} key
   * @param {string|number} value
   * @returns {Promise<number>}  - New string length after append
   * @throws {RampageError}
   *
   * @example
   * const len = await client.append('log', ' new entry'); // 10
   */
  async append(key, value) {
    const raw = await this._send(`APPEND ${quoteIfNeeded(key)} ${quoteIfNeeded(value)}`);
    return parseInteger(raw);
  }

  /**
   * STRLEN key
   * Returns the length of the string value stored at key.
   *
   * @param {string} key
   * @returns {Promise<number>}
   * @throws {RampageError}
   *
   * @example
   * const len = await client.strlen('name'); // 5
   */
  async strlen(key) {
    const raw = await this._send(`STRLEN ${quoteIfNeeded(key)}`);
    return parseInteger(raw);
  }

  // ─── List Commands ────────────────────────────────────────────────────────

  /**
   * LPUSH key value [EX seconds]
   * Pushes a value to the HEAD (left) of a list. Creates the list if needed.
   *
   * @param {string} key
   * @param {string|number} value
   * @param {object}  [options]
   * @param {number}  [options.ttl]  - Expiry in seconds for the list
   * @returns {Promise<number>}       - New list length
   * @throws {RampageError}
   *
   * @example
   * const len = await client.lpush('queue', 'task-1'); // 1
   */
  async lpush(key, value, options = {}) {
    const parts = ['LPUSH', quoteIfNeeded(key), quoteIfNeeded(value)];
    if (options.ttl != null) parts.push(String(options.ttl));
    const raw = await this._send(parts.join(' '));
    return parseInteger(raw);
  }

  /**
   * RPUSH key value [EX seconds]
   * Pushes a value to the TAIL (right) of a list. Creates the list if needed.
   *
   * @param {string} key
   * @param {string|number} value
   * @param {object}  [options]
   * @param {number}  [options.ttl]  - Expiry in seconds for the list
   * @returns {Promise<number>}       - New list length
   * @throws {RampageError}
   *
   * @example
   * const len = await client.rpush('queue', 'task-2'); // 2
   */
  async rpush(key, value, options = {}) {
    const parts = ['RPUSH', quoteIfNeeded(key), quoteIfNeeded(value)];
    if (options.ttl != null) parts.push(String(options.ttl));
    const raw = await this._send(parts.join(' '));
    return parseInteger(raw);
  }

  /**
   * LPOP key
   * Removes and returns the HEAD (left) element of a list.
   *
   * @param {string} key
   * @returns {Promise<string>}
   * @throws {RampageError}  - KEY_NOT_FOUND if key or list is empty/missing
   *
   * @example
   * const task = await client.lpop('queue'); // 'task-1'
   */
  async lpop(key) {
    const raw = await this._send(`LPOP ${quoteIfNeeded(key)}`);
    const payload = parseValue(raw);
    return payload === '' ? null : payload;
  }

  /**
   * RPOP key
   * Removes and returns the TAIL (right) element of a list.
   *
   * @param {string} key
   * @returns {Promise<string>}
   * @throws {RampageError}  - KEY_NOT_FOUND if key or list is empty/missing
   *
   * @example
   * const task = await client.rpop('queue'); // 'task-2'
   */
  async rpop(key) {
    const raw = await this._send(`RPOP ${quoteIfNeeded(key)}`);
    const payload = parseValue(raw);
    return payload === '' ? null : payload;
  }

  /**
   * LLEN key
   * Returns the length of a list.
   *
   * @param {string} key
   * @returns {Promise<number>}
   * @throws {RampageError}
   *
   * @example
   * const n = await client.llen('queue'); // 2
   */
  async llen(key) {
    const raw = await this._send(`LLEN ${quoteIfNeeded(key)}`);
    return parseInteger(raw);
  }

  /**
   * LINDEX key index
   * Returns the element at the given index in a list. Negative indices count from the tail.
   *
   * @param {string} key
   * @param {number} index
   * @returns {Promise<string>}
   * @throws {RampageError}  - KEY_NOT_FOUND if key missing, OUT_OF_BOUNDS if index invalid
   *
   * @example
   * const first = await client.lindex('queue', 0);  // 'task-1'
   * const last  = await client.lindex('queue', -1); // 'task-2'
   */
  async lindex(key, index) {
    const raw = await this._send(`LINDEX ${quoteIfNeeded(key)} ${index}`);
    const payload = parseValue(raw);
    return payload === '' ? null : payload;
  }

  /**
   * LSET key index value
   * Sets the element at index to value.
   *
   * @param {string} key
   * @param {number} index
   * @param {string|number} value
   * @returns {Promise<string>}  - 'Element set'
   * @throws {RampageError}
   *
   * @example
   * await client.lset('queue', 0, 'urgent-task');
   */
  async lset(key, index, value) {
    const raw = await this._send(`LSET ${quoteIfNeeded(key)} ${index} ${quoteIfNeeded(value)}`);
    return parseSimple(raw);
  }

  /**
   * LRANGE key start stop
   * Returns a JS array of elements from start to stop (inclusive). Negative
   * indices count from the tail (-1 = last element).
   *
   * @param {string} key
   * @param {number} start
   * @param {number} stop
   * @returns {Promise<string[]>}
   * @throws {RampageError}
   *
   * @example
   * const all = await client.lrange('queue', 0, -1); // ['task-1', 'task-2']
   */
  async lrange(key, start, stop) {
    const raw = await this._send(`LRANGE ${quoteIfNeeded(key)} ${start} ${stop}`);
    return parseLrange(raw);
  }

  // ─── Escape Hatch ─────────────────────────────────────────────────────────

  /**
   * sendCommand(rawCommand)
   *
   * Send any raw command string directly to RAMpage and get the raw response back
   * as a string — no parsing, no throwing on ERR. Useful for commands not yet in
   * the SDK or for debugging.
   *
   * @param {string} command  - The full command string, e.g. 'SET foo bar'
   * @returns {Promise<string>}
   *
   * @example
   * const resp = await client.sendCommand('SET mykey 42');
   * console.log(resp); // 'Key set successfully'
   */
  async sendCommand(command) {
    const raw = await this._send(command);
    return parseRaw(raw);
  }
}
