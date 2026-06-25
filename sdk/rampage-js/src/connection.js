import net from 'net';
import { EventEmitter } from 'events';
import { RampageError } from './errors.js';

/**
 * RampageConnection
 *
 * Manages a single persistent TCP connection to the RAMpage server.
 * Maintains an in-order request queue so that multiple concurrent SDK calls
 * each get their own response matched correctly (same pattern as server_persistent.js).
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
    this._destroyed = false; // set to true when disconnect() is called explicitly

    // Per-client line buffer — TCP streams data in chunks, we buffer until \n
    this._buffer = '';

    // FIFO queue of pending promises { resolve, reject }
    // Each sendCommand() call pushes one entry; each \n response pops one.
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
   * @returns {Promise<void>} Resolves when connected, rejects on timeout/error.
   */
  connect() {
    if (this._connected) return Promise.resolve();
    if (this._destroyed) {
      return Promise.reject(new RampageError('Client has been destroyed. Create a new client.', 'CONNECTION_ERROR'));
    }
    return this._doConnect();
  }

  /**
   * Send a raw command string to the server.
   * The \n delimiter is added automatically — do NOT include it.
   *
   * @param {string} command  - e.g. 'SET foo bar'
   * @returns {Promise<string>} Resolves with the raw response line from RAMpage
   */
  sendCommand(command) {
    return new Promise((resolve, reject) => {
      if (!this._connected || !this._socket) {
        return reject(new RampageError('Not connected to RAMpage. Did you call client.connect()?', 'CONNECTION_ERROR'));
      }

      // Queue this promise so the data handler can resolve it in order
      this._queue.push({ resolve, reject });

      // Write command + newline (protocol delimiter)
      this._socket.write(command + '\n');
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
      if (!this._socket || !this._connected) {
        resolve();
        return;
      }

      this._socket.once('close', resolve);
      this._socket.destroy();
    });
  }

  // ─── Private ──────────────────────────────────────────────────────────────

  _doConnect(isRetry = false) {
    return new Promise((resolve, reject) => {
      const socket = new net.Socket();
      // Guard so we only call resolve/reject once per attempt
      let settled = false;
      const settle = (fn, val) => { if (!settled) { settled = true; fn(val); } };

      // ── Timeout guard ─────────────────────────────────────────────────────
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

      // ── Connected ─────────────────────────────────────────────────────────
      socket.once('connect', () => {
        clearTimeout(timeoutId);
        this._socket = socket;
        this._connected = true;
        this._retryCount = 0;
        this.emit('connect');
        settle(resolve, undefined);
      });

      // ── Incoming data — buffer and process complete lines ─────────────────
      socket.on('data', (chunk) => {
        this._buffer += chunk.toString();

        let newlineIdx;
        while ((newlineIdx = this._buffer.indexOf('\n')) !== -1) {
          const line = this._buffer.substring(0, newlineIdx);
          this._buffer = this._buffer.substring(newlineIdx + 1);

          const pending = this._queue.shift();
          if (pending) {
            pending.resolve(line); // raw line, parser handles it upstream
          }
        }
      });

      // ── Socket error ──────────────────────────────────────────────────────
      // Note: on a refused connection, 'error' fires then 'close' fires.
      // We use a flag so reconnect is only scheduled once.
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

      // ── Socket closed ─────────────────────────────────────────────────────
      socket.on('close', () => {
        this._connected = false;
        this._buffer = '';

        // Only drain + emit if close wasn't already handled by the error path
        if (!reconnectScheduled) {
          this._drainQueueWithError(new RampageError('Connection closed unexpectedly', 'CONNECTION_ERROR'));
          this.emit('close');
          settle(reject, new RampageError('Connection closed unexpectedly', 'CONNECTION_ERROR'));
        } else {
          // Still emit close so users can observe it
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

  /**
   * Schedules a reconnect attempt with exponential backoff.
   */
  _scheduleReconnect() {
    if (this._destroyed) return;
    if (this.maxRetries !== -1 && this._retryCount >= this.maxRetries) {
      this.emit('error', new RampageError(
        `Max reconnect attempts (${this.maxRetries}) exceeded`,
        'MAX_RETRIES_EXCEEDED'
      ));
      return;
    }

    const delay = this.retryDelay * Math.pow(2, this._retryCount); // exponential backoff
    this._retryCount++;

    this.emit('reconnecting', { attempt: this._retryCount, delay });

    this._retryTimer = setTimeout(() => {
      this._retryTimer = null;
      this._doConnect(true).catch(() => {
        // _scheduleReconnect will be called again from inside _doConnect's error handler
      });
    }, delay);
  }

  /**
   * Reject all queued pending promises — called when the connection drops mid-flight.
   * @param {RampageError} err
   */
  _drainQueueWithError(err) {
    for (const pending of this._queue) {
      pending.reject(err);
    }
    this._queue = [];
  }
}
