// General utility functions for decimal formatting, and file operations
import fs from 'fs';
import child_process from 'child_process';

/** Ensures numeric values have decimal point for OSM XML compatibility */
const ensureDecimal = (i) => {
  if (parseInt(i) === i) return i.toFixed(1);
  else return i;
};

/**
 * Returns a promise that all osrm binaries are present and in good working order.
 * @param {Env} env
 */
function verifyExistenceOfBinaries(env) {
  for (const binPath of env.requiredBinaries) {
    if (!fs.existsSync(binPath)) {
      return Promise.reject(new Error(`${binPath} is missing. Build failed?`));
    }
    const res = child_process.spawnSync(binPath, ['--help']);
    if (res.error) {
      return Promise.reject(res.error);
    };
  };
  return Promise.resolve();
}

export { ensureDecimal, verifyExistenceOfBinaries };
