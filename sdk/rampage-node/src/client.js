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
 * RampageClient
 *
 * Redis-like client for RAMpage. Every method maps to the corresponding RESP
 * command, sends it over a persistent connection as a RESP array, and returns
 * a typed result — exactly like the official `redis` npm package.
 *
 * Usage:
 *   import { createClient } from 'rampage-node';
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

    // Forward connection events to the client surface
    this._conn.on('connect',      () =>  this.emit('connect'));
    this._conn.on('error',   (err) =>   this.emit('error', err));
    this._conn.on('close',        () =>  this.emit('close'));
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
   * Low-level: send a pre-split arg list as a RESP command.
   * @param {string[]} args - e.g. ['SET', 'foo', 'bar']
   * @returns {Promise<any>}
   */
  async _send(args) {
    return this._conn.sendCommand(args);
  }

  // ─── String Commands ──────────────────────────────────────────────────────

  /**
   * SET key value [EX seconds]
   * Sets a key to a string value with an optional TTL in seconds.
   *
   * @param {string} key
   * @param {string|number} value
   * @param {object}  [options]
   * @param {number}  [options.ttl]  - Expiry in seconds
   * @returns {Promise<string>}       - 'OK'
   * @throws {RampageError}
   *
   * @example
   * await client.set('name', 'Alice');
   * await client.set('token', 'abc123', { ttl: 60 });
   */
  async set(key, value, options = {}) {
    const args = ['SET', String(key), String(value)];
    if (options.ttl != null) args.push(String(options.ttl));
    return parseSimple(await this._send(args));
  }

  /**
   * GET key
   * Gets the string value of a key.
   *
   * @param {string} key
   * @returns {Promise<string | null>}  - The value, or null if the key doesn't exist
   * @throws {RampageError}
   *
   * @example
   * const val = await client.get('name'); // 'Alice'
   */
  async get(key) {
    return parseValue(await this._send(['GET', String(key)]));
  }

  /**
   * DEL key
   * Deletes a key. Returns 1 if the key existed and was deleted, 0 if not.
   *
   * @param {string} key
   * @returns {Promise<number>}  - 1 (deleted) or 0 (not found)
   * @throws {RampageError}
   *
   * @example
   * const deleted = await client.del('name'); // 1
   */
  async del(key) {
    return parseInteger(await this._send(['DEL', String(key)]));
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
    return parseTtl(await this._send(['TTL', String(key)]));
  }

  /**
   * EXPIRE key seconds
   * Sets or updates the TTL on an existing key.
   * Returns 1 if set, 0 if key does not exist.
   *
   * @param {string} key
   * @param {number} seconds
   * @returns {Promise<number>}  - 1 (set) or 0 (key not found)
   * @throws {RampageError}
   *
   * @example
   * await client.expire('token', 120);
   */
  async expire(key, seconds) {
    return parseInteger(await this._send(['EXPIRE', String(key), String(seconds)]));
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
    return parseInteger(await this._send(['APPEND', String(key), String(value)]));
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
    return parseInteger(await this._send(['STRLEN', String(key)]));
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
    const args = ['LPUSH', String(key), String(value)];
    if (options.ttl != null) args.push(String(options.ttl));
    return parseInteger(await this._send(args));
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
    const args = ['RPUSH', String(key), String(value)];
    if (options.ttl != null) args.push(String(options.ttl));
    return parseInteger(await this._send(args));
  }

  /**
   * LPOP key
   * Removes and returns the HEAD (left) element of a list.
   *
   * @param {string} key
   * @returns {Promise<string | null>}  - The element, or null if key doesn't exist
   * @throws {RampageError}
   *
   * @example
   * const task = await client.lpop('queue'); // 'task-1'
   */
  async lpop(key) {
    return parseValue(await this._send(['LPOP', String(key)]));
  }

  /**
   * RPOP key
   * Removes and returns the TAIL (right) element of a list.
   *
   * @param {string} key
   * @returns {Promise<string | null>}  - The element, or null if key doesn't exist
   * @throws {RampageError}
   *
   * @example
   * const task = await client.rpop('queue'); // 'task-2'
   */
  async rpop(key) {
    return parseValue(await this._send(['RPOP', String(key)]));
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
    return parseInteger(await this._send(['LLEN', String(key)]));
  }

  /**
   * LINDEX key index
   * Returns the element at the given index. Negative indices count from the tail.
   *
   * @param {string} key
   * @param {number} index
   * @returns {Promise<string | null>}  - The element, or null if index out of range
   * @throws {RampageError}
   *
   * @example
   * const first = await client.lindex('queue', 0);  // 'task-1'
   * const last  = await client.lindex('queue', -1); // 'task-2'
   */
  async lindex(key, index) {
    return parseValue(await this._send(['LINDEX', String(key), String(index)]));
  }

  /**
   * LSET key index value
   * Sets the element at index to value.
   *
   * @param {string} key
   * @param {number} index
   * @param {string|number} value
   * @returns {Promise<string>}  - 'OK'
   * @throws {RampageError}
   *
   * @example
   * await client.lset('queue', 0, 'urgent-task');
   */
  async lset(key, index, value) {
    return parseSimple(await this._send(['LSET', String(key), String(index), String(value)]));
  }

  /**
   * LRANGE key start stop
   * Returns a JS array of elements from start to stop (inclusive).
   * Negative indices count from the tail (-1 = last element).
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
    return parseLrange(await this._send(['LRANGE', String(key), String(start), String(stop)]));
  }

  // ─── Escape Hatch ─────────────────────────────────────────────────────────

  /**
   * sendCommand(args)
   *
   * Send any RESP command directly and get the raw decoded value back.
   * Useful for commands not yet in the SDK or for debugging.
   *
   * @param {string[]} args  - e.g. ['SET', 'mykey', '42']
   * @returns {Promise<any>}
   *
   * @example
   * const value = await client.sendCommand(['SET', 'mykey', '42']);
   * console.log(value); // 'OK'
   */
  async sendCommand(args) {
    return parseRaw(await this._send(args));
  }
}
