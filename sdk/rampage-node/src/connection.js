import net from 'net';
import { EventEmitter } from 'events';
import { RampageError } from './errors.js';

/**
 * RespReader
 *
 * Stateful RESP response parser attached to a single TCP socket.
 * RAMpage now speaks RESP (same wire format as Redis), so each response
 * is one of:
 *
 *   +<str>\r\n         Simple String  → resolves with the string
 *   -<err>\r\n         Error          → rejects with RampageError
 *   :<n>\r\n           Integer        → resolves with the number
 *   $<len>\r\n<data>\r\n  Bulk String → resolves with string (or null for $-1)
 *   *<n>\r\n ...       Array          → resolves with string[]
 *
 * The reader accumulates raw socket chunks into an internal buffer and
 * tries to extract one complete RESP value at a time, resolving the
 * next pending promise in the queue when one is ready.
 */
class RespReader {
  constructor() {
    this._buf = ''; // raw byte accumulation buffer
  }

  /** Append a raw chunk from socket.on('data') */
  feed(chunk) {
    this._buf += chunk.toString();
  }

  /** Reset buffer (e.g. on reconnect) */
  reset() {
    this._buf = '';
  }

  /**
   * Try to parse one complete RESP value from the internal buffer.
   * Returns { value, consumed: true } on success, or { consumed: false } if
   * more data is needed.
   *
   * Never throws — errors are surfaced as { isError: true, message, code }.
   */
  tryParse() {
    if (this._buf.length === 0) return { consumed: false };

    const firstByte = this._buf[0];

    if (firstByte === '+') return this._parseSimple();
    if (firstByte === '-') return this._parseError();
    if (firstByte === ':') return this._parseInteger();
    if (firstByte === '$') return this._parseBulk();
    if (firstByte === '*') return this._parseArray();

    // Unknown byte — likely leftover \r or junk; skip one character
    this._buf = this._buf.slice(1);
    return { consumed: false };
  }

  // ─── Primitive parsers ───────────────────────────────────────────────────

  _readLine() {
    const idx = this._buf.indexOf('\r\n');
    if (idx === -1) return null;        // incomplete
    const line = this._buf.slice(1, idx); // strip the type byte at [0]
    this._buf = this._buf.slice(idx + 2);
    return line;
  }

  _parseSimple() {
    const line = this._readLine();
    if (line === null) return { consumed: false };
    return { consumed: true, value: line };
  }

  _parseError() {
    const line = this._readLine();
    if (line === null) return { consumed: false };
    // RESP errors start with "ERR ", "WRONGTYPE ", etc. — strip the prefix.
    const msg = line.replace(/^(ERR|WRONGTYPE)\s+/i, '');
    let code = 'ERR';
    const lower = msg.toLowerCase();
    if (lower.includes('not found'))          code = 'KEY_NOT_FOUND';
    else if (lower.includes('wrongtype') || lower.includes('wrong type')) code = 'WRONG_TYPE';
    else if (lower.includes('wrong number') || lower.includes('wrong args')) code = 'WRONG_ARGS';
    else if (lower.includes('unknown command')) code = 'UNKNOWN_COMMAND';
    else if (lower.includes('out of bounds') || lower.includes('out of range')) code = 'OUT_OF_BOUNDS';
    return { consumed: true, isError: true, message: msg, code };
  }

  _parseInteger() {
    const line = this._readLine();
    if (line === null) return { consumed: false };
    return { consumed: true, value: Number(line) };
  }

  _parseBulk() {
    // Need at least the length line
    const crIdx = this._buf.indexOf('\r\n');
    if (crIdx === -1) return { consumed: false };

    const len = parseInt(this._buf.slice(1, crIdx), 10);

    if (len === -1) {
      // Null bulk string ($-1\r\n) — key not found, return null
      this._buf = this._buf.slice(crIdx + 2);
      return { consumed: true, value: null };
    }

    // Need: length-line (\r\n) + len bytes of data + trailing \r\n
    const needed = crIdx + 2 + len + 2;
    if (this._buf.length < needed) return { consumed: false };

    const data = this._buf.slice(crIdx + 2, crIdx + 2 + len);
    this._buf = this._buf.slice(needed);
    return { consumed: true, value: data };
  }

  _parseArray() {
    const crIdx = this._buf.indexOf('\r\n');
    if (crIdx === -1) return { consumed: false };

    const count = parseInt(this._buf.slice(1, crIdx), 10);
    // Save a snapshot in case we need to roll back (incomplete array)
    const snapshot = this._buf;
    this._buf = this._buf.slice(crIdx + 2);

    if (count === -1) return { consumed: true, value: null };  // null array
    if (count === 0)  return { consumed: true, value: [] };

    const items = [];
    for (let i = 0; i < count; i++) {
      const result = this.tryParse();
      if (!result.consumed) {
        // Not enough data yet — roll back the whole array attempt
        this._buf = snapshot;
        return { consumed: false };
      }
      if (result.isError) {
        this._buf = snapshot;
        return result; // bubble up error
      }
      items.push(result.value);
    }

    return { consumed: true, value: items };
  }
}

// ─────────────────────────────────────────────────────────────────────────────

/**
 * RampageConnection
 *
 * Manages a single persistent TCP connection to the RAMpage server.
 * Sends commands as RESP arrays (*N\r\n$len\r\n...) and reads RESP responses.
 * Maintains an in-order request queue so concurrent SDK calls each get their
 * own correctly matched response.
 *
 * Emits:
 *   'connect'        — fired once the socket is established
 *   'error' (err)    — fired on socket errors
 *   'close'          — fired when the connection is closed
 *   'reconnecting'   — fired before each reconnect attempt
 */
export class RampageConnection extends EventEmitter {
  /**
   * @param {object} options
   * @param {string}  options.host           - RAMpage server host (default: '127.0.0.1')
   * @param {number}  options.port           - RAMpage server port (default: 2006)
   * @param {boolean} options.autoReconnect  - Auto-reconnect on close (default: true)
   * @param {number}  options.maxRetries     - Max reconnect attempts (default: 5, -1 = infinite)
   * @param {number}  options.retryDelay     - Base delay in ms for exponential backoff (default: 500)
   * @param {number}  options.connectTimeout - ms to wait for initial connect (default: 5000)
   */
  constructor(options = {}) {
    super();
    this.host = options.host ?? '127.0.0.1';
    this.port = options.port ?? 2006;
    this.autoReconnect = options.autoReconnect ?? true;
    this.maxRetries = options.maxRetries ?? 5;
    this.retryDelay = options.retryDelay ?? 500;
    this.connectTimeout = options.connectTimeout ?? 5000;

    /** @type {net.Socket | null} */
    this._socket = null;
    this._connected = false;
    this._destroyed = false;

    this._reader = new RespReader();

    // FIFO queue of pending promises { resolve, reject }
    this._queue = [];

    this._retryCount = 0;
    this._retryTimer = null;
  }

  // ─── Public ───────────────────────────────────────────────────────────────

  get isConnected() {
    return this._connected;
  }

  /**
   * Establish the TCP connection.
   * @returns {Promise<void>}
   */
  connect() {
    if (this._connected) return Promise.resolve();
    if (this._destroyed) {
      return Promise.reject(new RampageError('Client has been destroyed. Create a new client.', 'CONNECTION_ERROR'));
    }
    return this._doConnect();
  }

  /**
   * Send a command as a RESP array to the server.
   * `args` is the already-split token list, e.g. ['SET', 'foo', 'bar'].
   *
   * @param {string[]} args
   * @returns {Promise<any>} Resolves with the parsed RESP value (string | number | null | string[])
   */
  sendCommand(args) {
    return new Promise((resolve, reject) => {
      if (!this._connected || !this._socket) {
        return reject(new RampageError('Not connected to RAMpage. Did you call client.connect()?', 'CONNECTION_ERROR'));
      }

      this._queue.push({ resolve, reject });

      // Encode as RESP Array: *N\r\n$len\r\narg\r\n ...
      let frame = `*${args.length}\r\n`;
      for (const arg of args) {
        const s = String(arg);
        frame += `$${Buffer.byteLength(s)}\r\n${s}\r\n`;
      }
      this._socket.write(frame);
    });
  }

  /**
   * Gracefully close the connection. No auto-reconnect after this.
   * @returns {Promise<void>}
   */
  disconnect() {
    this._destroyed = true;
    this.autoReconnect = false;

    if (this._retryTimer) {
      clearTimeout(this._retryTimer);
      this._retryTimer = null;
    }

    return new Promise((resolve) => {
      if (!this._socket || !this._connected) { resolve(); return; }
      this._socket.once('close', resolve);
      this._socket.destroy();
    });
  }

  // ─── Private ──────────────────────────────────────────────────────────────

  _doConnect(isRetry = false) {
    return new Promise((resolve, reject) => {
      const socket = new net.Socket();
      let settled = false;
      const settle = (fn, val) => { if (!settled) { settled = true; fn(val); } };

      const timeoutId = setTimeout(() => {
        socket.destroy();
        const err = new RampageError(
          `Connection to ${this.host}:${this.port} timed out after ${this.connectTimeout}ms`,
          'CONNECTION_TIMEOUT'
        );
        this.emit('error', err);
        settle(reject, err);
        if (!this._destroyed) this._scheduleReconnect();
      }, this.connectTimeout);

      socket.once('connect', () => {
        clearTimeout(timeoutId);
        this._socket = socket;
        this._connected = true;
        this._retryCount = 0;
        this._reader.reset();
        this.emit('connect');
        settle(resolve, undefined);
      });

      // ── Incoming data — feed the RESP reader and drain pending promises ──
      socket.on('data', (chunk) => {
        this._reader.feed(chunk);

        // Keep parsing as long as there is a complete value available
        // (handles pipelining: multiple responses in one TCP packet)
        let result;
        while (this._queue.length > 0) {
          result = this._reader.tryParse();
          if (!result.consumed) break;

          const pending = this._queue.shift();
          if (result.isError) {
            pending.reject(new RampageError(result.message, result.code));
          } else {
            pending.resolve(result.value);
          }
        }
      });

      let reconnectScheduled = false;

      socket.on('error', (err) => {
        clearTimeout(timeoutId);
        this._connected = false;
        const rampageErr = new RampageError(err.message, 'CONNECTION_ERROR');
        this._drainQueueWithError(rampageErr);
        this.emit('error', rampageErr);
        settle(reject, rampageErr);
        if (!this._destroyed && !reconnectScheduled) {
          reconnectScheduled = true;
          this._scheduleReconnect();
        }
      });

      socket.on('close', () => {
        this._connected = false;
        this._reader.reset();
        if (!reconnectScheduled) {
          this._drainQueueWithError(new RampageError('Connection closed unexpectedly', 'CONNECTION_ERROR'));
          this.emit('close');
          settle(reject, new RampageError('Connection closed unexpectedly', 'CONNECTION_ERROR'));
        } else {
          this.emit('close');
        }
        if (!this._destroyed && this.autoReconnect && !reconnectScheduled) {
          reconnectScheduled = true;
          this._scheduleReconnect();
        }
      });

      socket.connect(this.port, this.host);
    });
  }

  _scheduleReconnect() {
    if (this._destroyed) return;
    if (this.maxRetries !== -1 && this._retryCount >= this.maxRetries) {
      this.emit('error', new RampageError(
        `Max reconnect attempts (${this.maxRetries}) exceeded`,
        'MAX_RETRIES_EXCEEDED'
      ));
      return;
    }
    const delay = this.retryDelay * Math.pow(2, this._retryCount);
    this._retryCount++;
    this.emit('reconnecting', { attempt: this._retryCount, delay });
    this._retryTimer = setTimeout(() => {
      this._retryTimer = null;
      this._doConnect(true).catch(() => {});
    }, delay);
  }

  _drainQueueWithError(err) {
    for (const pending of this._queue) pending.reject(err);
    this._queue = [];
  }
}
