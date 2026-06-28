import { RampageError } from './errors.js';

/**
 * parser.js — typed result helpers for rampage-node
 *
 * The RESP wire-level parsing (Simple Strings, Integers, Bulk Strings,
 * Arrays, Errors) is now done by RespReader inside connection.js.
 * By the time these helpers are called, `raw` is already a resolved JS
 * value (string | number | null | string[]) — NOT a raw protocol string.
 *
 * These functions exist to:
 *   1. Apply type assertions (guard against unexpected server behaviour).
 *   2. Provide clear TypeScript-style contracts for each command family.
 *   3. Keep client.js readable by naming the intent ("parseInteger", etc.).
 */

/**
 * Assert a value is a string (Simple String responses: SET, DEL void, LSET …)
 * Returns the string. For SET/DEL the value will be "OK".
 *
 * @param {any} value - Already-decoded RESP value from RespReader
 * @returns {string}
 * @throws {RampageError}
 */
export function parseSimple(value) {
  if (typeof value === 'string') return value;
  if (typeof value === 'number') return String(value); // e.g. DEL returns integer 1/0
  throw new RampageError(
    `Expected simple/string response, got: ${JSON.stringify(value)}`,
    'PARSE_ERROR'
  );
}

/**
 * Assert a value is a string or null (Bulk String responses: GET, LPOP, RPOP, LINDEX …)
 * null means the key was not found ($-1 from server).
 *
 * @param {any} value
 * @returns {string | null}
 * @throws {RampageError}
 */
export function parseValue(value) {
  if (value === null) return null;
  if (typeof value === 'string') return value;
  throw new RampageError(
    `Expected bulk-string response, got: ${JSON.stringify(value)}`,
    'PARSE_ERROR'
  );
}

/**
 * Assert a value is a number (Integer responses: LLEN, STRLEN, LPUSH, RPUSH, DEL …)
 *
 * @param {any} value
 * @returns {number}
 * @throws {RampageError}
 */
export function parseInteger(value) {
  if (typeof value === 'number' && Number.isFinite(value)) return value;
  throw new RampageError(
    `Expected integer response, got: ${JSON.stringify(value)}`,
    'PARSE_ERROR'
  );
}

/**
 * Parse TTL responses.
 *   -2 → key does not exist
 *   -1 → key has no expiry
 *    N → seconds remaining
 *
 * @param {any} value
 * @returns {number}
 * @throws {RampageError}
 */
export function parseTtl(value) {
  return parseInteger(value);
}

/**
 * Parse LRANGE responses — an array of bulk strings.
 * Empty array if no elements matched.
 *
 * @param {any} value
 * @returns {string[]}
 * @throws {RampageError}
 */
export function parseLrange(value) {
  if (Array.isArray(value)) return value;
  if (value === null) return [];
  throw new RampageError(
    `Expected array response, got: ${JSON.stringify(value)}`,
    'PARSE_ERROR'
  );
}

/**
 * Raw pass-through for sendCommand() escape hatch.
 * Returns the value as-is, stringified if necessary.
 *
 * @param {any} value
 * @returns {any}
 */
export function parseRaw(value) {
  return value;
}
