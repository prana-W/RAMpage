import { RampageError } from './errors.js';

const SUCC_PREFIX = 'SUCC:';
const ERR_PREFIX  = 'ERR:';

/**
 * Split a raw TCP response line into { ok: boolean, payload: string }.
 * Throws RampageError immediately if the line is an ERR response.
 *
 * @param {string} raw
 * @returns {string} payload — the part after "SUCC:"
 * @throws {RampageError}
 */
function unwrap(raw) {
  const line = raw.trim();

  if (line.startsWith(ERR_PREFIX)) {
    const message = line.slice(ERR_PREFIX.length);
    // Derive a machine-readable code from the message text
    let code = 'ERR';
    const lower = message.toLowerCase();
    if (lower.includes('not found'))        code = 'KEY_NOT_FOUND';
    else if (lower.includes('wrong type'))  code = 'WRONG_TYPE';
    else if (lower.includes('wrong number') || lower.includes('wrong number')) code = 'WRONG_ARGS';
    else if (lower.includes('unknown command')) code = 'UNKNOWN_COMMAND';
    else if (lower.includes('out of bounds'))   code = 'OUT_OF_BOUNDS';
    else if (lower.includes('list is empty'))   code = 'KEY_NOT_FOUND';
    else if (lower.includes('ttl must'))    code = 'WRONG_ARGS';
    else if (lower.includes('must be an integer')) code = 'WRONG_ARGS';
    throw new RampageError(message, code);
  }

  if (line.startsWith(SUCC_PREFIX)) {
    return line.slice(SUCC_PREFIX.length); // return the raw payload, callers type-cast it
  }

  // Should never happen with a compliant server — surface it clearly
  throw new RampageError(
    `Unexpected response format (missing SUCC:/ERR: prefix): "${line}"`,
    'PARSE_ERROR'
  );
}

/**
 * Parse void success responses (SET, DEL, EXPIRE, LSET).
 * Returns the payload string (usually empty ""), throws on ERR.
 *
 * @param {string} raw
 * @returns {string}  — empty string for most void ops, or a status message
 * @throws {RampageError}
 */
export function parseSimple(raw) {
  return unwrap(raw); // payload is a plain (usually empty) string
}

/**
 * Parse value responses (GET, LPOP, RPOP, LINDEX).
 * Returns the string value, or null if the SUCC payload is empty
 * (which the server uses to signal a key-not-found on value-returning commands — but
 * since we now use ERR: for not-found, SUCC: with empty payload means an actual empty string).
 * In practice the server will return ERR:Key not found so this always has a real value on SUCC.
 *
 * @param {string} raw
 * @returns {string}
 * @throws {RampageError}
 */
export function parseValue(raw) {
  const payload = unwrap(raw);
  return payload; // always a real string — server returns ERR: for missing keys
}

/**
 * Parse integer responses (APPEND length, STRLEN, LLEN, LPUSH/RPUSH new size, TTL).
 *
 * @param {string} raw
 * @returns {number}
 * @throws {RampageError}
 */
export function parseInteger(raw) {
  const payload = unwrap(raw);
  const n = Number(payload);
  if (Number.isFinite(n)) return n;
  throw new RampageError(
    `Expected integer payload, got: "${payload}"`,
    'PARSE_ERROR'
  );
}

/**
 * Parse TTL responses.
 *   -2 → key does not exist  (server sends ERR:Key not found → caught by unwrap → RampageError)
 *   -1 → key has no expiry
 *    N → seconds remaining
 *
 * @param {string} raw
 * @returns {number}
 * @throws {RampageError}
 */
export function parseTtl(raw) {
  return parseInteger(raw); // TTL payload is always an integer
}

/**
 * Parse LRANGE responses.
 * Server sends items pipe-separated in the SUCC payload: "foo|bar|baz"
 * Empty list → SUCC: (empty payload) → returns []
 *
 * @param {string} raw
 * @returns {string[]}
 * @throws {RampageError}
 */
export function parseLrange(raw) {
  const payload = unwrap(raw);
  if (payload === '') return [];
  return payload.split('|');
}

/**
 * Raw pass-through for sendCommand().
 * Returns the full raw line (with prefix stripped if present, otherwise as-is).
 * Never throws — lets the caller decide what to do.
 *
 * @param {string} raw
 * @returns {string}
 */
export function parseRaw(raw) {
  const line = raw.trim();
  if (line.startsWith(SUCC_PREFIX)) return line.slice(SUCC_PREFIX.length);
  if (line.startsWith(ERR_PREFIX))  return line; // keep "ERR:..." visible to the caller
  return line;
}
