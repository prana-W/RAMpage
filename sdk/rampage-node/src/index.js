/**
 * rampage-node
 * Official Node.js SDK for RAMpage — a high-performance in-memory database.
 * Speaks the RESP (REdis Serialization Protocol) wire format natively.
 *
 * @example
 * import { createClient } from 'rampage-node';
 *
 * const client = createClient({ host: '127.0.0.1', port: 2006 });
 * await client.connect();
 * await client.set('name', 'Alice');
 * const name = await client.get('name'); // 'Alice'
 * await client.disconnect();
 */

export { RampageClient } from './client.js';
export { RampageError } from './errors.js';

/**
 * createClient(options?)
 *
 * Factory function — the idiomatic way to create a RAMpage client.
 * Mirrors the `redis` npm package's `createClient()` API.
 *
 * @param {object} [options]
 * @param {string}  [options.host='127.0.0.1']    - RAMpage server hostname or IP
 * @param {number}  [options.port=2006]            - RAMpage server port
 * @param {boolean} [options.autoReconnect=true]   - Auto-reconnect on disconnect
 * @param {number}  [options.maxRetries=5]         - Max auto-reconnect attempts (-1 = infinite)
 * @param {number}  [options.retryDelay=500]       - Base delay in ms for exponential backoff
 * @param {number}  [options.connectTimeout=5000]  - Timeout in ms for the initial connection
 * @returns {import('./client.js').RampageClient}
 *
 * @example
 * // Basic
 * const client = createClient();
 * await client.connect();
 *
 * @example
 * // Custom host/port
 * const client = createClient({ host: 'my-server.local', port: 2006 });
 * await client.connect();
 *
 * @example
 * // With event listeners
 * const client = createClient({ maxRetries: -1 });
 * client.on('connect', () => console.log('Connected to RAMpage!'));
 * client.on('error', (err) => console.error('RAMpage error:', err.message));
 * client.on('reconnecting', ({ attempt, delay }) =>
 *   console.log(`Reconnecting... attempt ${attempt} in ${delay}ms`)
 * );
 * await client.connect();
 */
import { RampageClient } from './client.js';

export function createClient(options = {}) {
  return new RampageClient(options);
}
