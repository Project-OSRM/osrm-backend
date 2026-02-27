// Process execution utilities for running OSRM binaries and managing subprocesses
import child_process from 'node:child_process';
import path from 'node:path';

import { env } from '../support/env.js';

/** Returns the full path to the binary. */
export function mkBinPath(bin) {
  return path.join(env.wp.buildPath, `${bin}${env.EXE}`);
}

/**
 * Runs an osrm binary in asynchronous mode.
 *
 * Use case: osrm-routed.
 *
 * @param {string}   bin     The name of the binary, eg. osrm-routed
 * @param {string[]} args    The arguments to the binary
 * @param {object}   options Options passed to the spawn function
 * @param {function} log     Function that consumes logs, eg world.log
 */
export function runBin(bin, args, options, log) {
  const cmd = mkBinPath(bin);
  const argsAsString = args.join(' ');
  log(`running ${bin} as:\n${cmd} ${argsAsString}`);

  const child = child_process.spawn(
    cmd,
    args,
    options,
  );

  // we MUST consume these or the osrm-routed process will block
  // we cannot send these to world.log() because output might happen between steps
  child.stderr.on('data', (data) => log(data));
  child.stdout.on('data', (data) => log(data));

  child.on('error', (err) => {
    log(`${bin} aborted with error ${err}`);
    throw(err);
  });
  child.on('exit', (code, signal) => {
    if (signal != null) {
      const msg = `${bin} aborted with signal ${child.signal}`;
      log(msg);
      throw new Error(msg);
    } else {
      log(`${bin} completed with exit code ${code}`);
    }
  });
  return child;
}

/**
 * Runs an osrm binary in synchronous mode.
 *
 * Use case: osrm-extract and friends.
 *
 * @param {string}   bin     The name of the binary, eg. osrm-extract
 * @param {string[]} args    The arguments to the binary
 * @param {object}   options Options passed to the spawnSync function
 * @param {function} log     Function that consumes logs, eg world.log
 */
export function runBinSync(bin, args, options, log) {
  const cmd = mkBinPath(bin);
  const argsAsString = args.join(' ');
  log(`running ${bin} as:\n${cmd} ${argsAsString}`);

  const child = child_process.spawnSync(
    cmd,
    args,
    options
  );
  if (child.stdout)
    log(`${bin} stdout:\n${child.stdout}`);
  if (child.stderr)
    log(`${bin} stderr:\n${child.stderr}`);
  if (child.status != null)
    log(`${bin} completed with exit code ${child.status}`);
  if (child.signal != null) {
    const msg = `${bin} aborted with signal ${child.signal}`;
    log(msg);
    if (child.signal != 'SIGABRT') // some tests deliberately fail
      throw new Error(msg);
  }
  return child;
}
