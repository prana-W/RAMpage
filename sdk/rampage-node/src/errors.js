/**
 * RampageError
 *
 * Thrown whenever RAMpage responds with an error status or the SDK
 * encounters a protocol/connection level issue.
 *
 * @property {string} code  - Short machine-readable code, e.g. 'ERR', 'KEY_NOT_FOUND', 'WRONG_TYPE', 'CONNECTION_ERROR'
 * @property {string} message - Human-readable description from the server or SDK
 */
export class RampageError extends Error {
  /**
   * @param {string} message
   * @param {string} [code='ERR']
   */
  constructor(message, code = 'ERR') {
    super(message);
    this.name = 'RampageError';
    this.code = code;

    // Preserve proper stack trace in V8
    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, RampageError);
    }
  }
}
