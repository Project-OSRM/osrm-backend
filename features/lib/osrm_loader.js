// osrm-routed process management and data loading strategies (datastore, mmap, direct)
import child_process from 'node:child_process';

import waitOn from 'wait-on';

import { env } from '../support/env.js';
import { runBinSync, mkBinPath } from '../support/run.js';

/**
 * A class for running osrm-routed. Subclasses implement alternate ways of data loading.
 */
class OSRMBaseLoader {
  constructor() {
    this.child = null;
  }

  /**
   * Starts OSRM server and waits for it to accept connections
   *
   * @param log A log function. Inside a scenario we can log to the world.log()
   * function, but world.log() cannot be used between scenarios. We must log
   * long-running processes to the global log.
   */
  spawn(scenario, args, log) {
    if (this.osrmIsRunning())
      throw new Error('osrm-routed already running!');

    const cmd = mkBinPath('osrm-routed');
    const argsAsString = args.join(' ');
    log(`running ${cmd} ${argsAsString}`);

    return new Promise((resolve) => {
      this.child = child_process.spawn(
        cmd,
        args,
        { env : scenario.environment },
      );

      this.child.stdout.on('data', (data) => {
        if (data.includes('running and waiting for requests')) {
          log('Routed running and waiting for requests');
          resolve();
        }
      });

      this.child.on('exit', (code) => {
        log(`osrm-routed completed with exit code ${code}`);
        this.child = null;
      });
    });
  }

  /**
   * Terminates the OSRM server process gracefully
   *
   * @returns A promise resolved when routed has terminated.
   */
  kill() {
    if (!this.osrmIsRunning())
      return Promise.resolve();
    return new Promise((resolve) => {
      this.child.on('close', resolve);
      this.child.kill();
    });
  }

  osrmIsRunning() {
    return this.child;
  }

  // public interface

  /** Called at the init of the cucumber run */
  beforeAll() { return Promise.resolve(); }
  /**
   * Called at the start of each scenario
   *
   * @returns A promise resolved when routed is ready for connections.
   */
  before(_scenario) {
    return Promise.resolve();
  }
  /**
   * Called at the end of each scenario
   *
   * @returns A promise resolved when routed has terminated.
   */
  after(_scenario) {
    return this.kill();
  }
  /** Called at the end of the cucumber run */
  afterAll() { return Promise.resolve(); }
}

/** This loader tells osrm-routed to load data directly from .osrm files into memory */
export class OSRMDirectLoader extends OSRMBaseLoader {
  before(scenario) {
    return this.spawn(scenario, [
      scenario.osrmCacheFile,
      '-p',
      env.wp.port,
      '-i',
      env.wp.ip,
      '-a',
      env.wp.algorithm.toUpperCase(),
    ].concat(scenario.loaderArgs),
    scenario.log);
  }
}

/** This loader tells osrm-routed to use memory-mapped files. */
export class OSRMmmapLoader extends OSRMBaseLoader {
  before(scenario) {
    return this.spawn(scenario, [
      scenario.osrmCacheFile,
      '-p',
      env.wp.port,
      '-i',
      env.wp.ip,
      '-a',
      env.wp.algorithm.toUpperCase(),
      '--mmap',
    ].concat(scenario.loaderArgs),
    scenario.log);
  }
}

/**
 * This loader keeps one and the same osrm-routed running for the whole cucumber run. It
 * uses osrm-datastore to load new data into osrm-routed.
 */
export class OSRMDatastoreLoader extends OSRMBaseLoader {
  constructor() {
    super();
    this.current_scenario = null;
  }

  /**
   * Custom log function
   *
   * For long-running osrm-routed switch the log to the current scenario
   */
  async logSync(msg) {
    if (this.current_scenario)
      await this.current_scenario.log(msg);
    else
      env.globalLog(msg);
  }

  before(scenario) {
    this.current_scenario = scenario;
    this.semaphore = new Promise((resolve) => this.resolve = resolve);
    runBinSync(
      'osrm-datastore',
      [
        '--dataset-name',
        env.DATASET_NAME,
        scenario.osrmCacheFile,
      ].concat(scenario.loaderArgs),
      { env : scenario.environment },
      scenario.log
    );

    if (this.osrmIsRunning())
      // When osrm-datastore exits, then osrm-routed may not yet have switched facades.
      // It would right now answer the query from the old dataset.  We return a promise
      // that is resolved when osrm-routed outputs "updated facade to regions ...".
      return this.semaphore;

    // workaround for annoying misfeature: if there are no datastores osrm-routed
    // chickens out, so we cannot just start osrm-routed in beforeAll where it naturally
    // belonged, but must load a datastore first.

    const args = [
      '--shared-memory',
      '--dataset-name',
      env.DATASET_NAME,
      '-p',
      env.wp.port,
      '-i',
      env.wp.ip,
      '-a',
      env.wp.algorithm.toUpperCase(),
    ]; // .concat(scenario.loaderArgs));

    const cmd = mkBinPath('osrm-routed');
    const argsAsString = args.join(' ');
    this.logSync(`running ${cmd} ${argsAsString}`);

    return new Promise((resolve) => {
      this.child = child_process.spawn(
        cmd,
        args,
        { env : scenario.environment },
      );

      this.child.stdout.on('data', (data) => {
        this.logSync(`osrm-routed stdout:\n${data}`);
        if (data.includes('updated facade')) {
          this.logSync('Facade updated and promise resolved');
          this.resolve();
        }
        else if (data.includes('running and waiting for requests')) {
          this.logSync('Routed running and waiting for requests');
          resolve();
        }
      });

      // we MUST consume stdout and stderr or the osrm-routed process will block eventually
      this.child.stderr.on('data', (data) => this.logSync(`osrm-routed stderr:\n${data}`));

      this.child.on('exit', (code, signal) => {
        this.child = null;
        if (signal != null) {
          const msg = `osrm-routed aborted with signal ${signal}`;
          this.logSync(msg);
          // throw new Error(msg);
        }
        if (code != null) {
          const msg = `osrm-routed completed with exit code ${code}`;
          this.logSync(msg);
          if (code != 0)
            throw new Error(msg);
        }
      });
    });
  }
  after () {
    this.current_scenario = null;
    return Promise.resolve();
  }
  afterAll () {
    this.current_scenario = null;
    return this.kill();
  }
}

/** throws error if osrm-routed is up */
// FIXME: don't bother if osrm-routed is already running, just use the next port
export function testOsrmDown() {
  const host = `${env.wp.ip}:${env.wp.port}`;
  const waitOptions = {
    resources: [`tcp:${host}`],
    delay:       0, // initial delay in ms
    interval:  100, // poll interval in ms
    timeout:  1000, // timeout in ms
    reverse: true,
  };
  return waitOn(waitOptions).catch(() => { throw new Error(
    `osrm-routed is already running on ${host}.`
  );});
}
