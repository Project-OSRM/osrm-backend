// Sets up global environment constants and configuration for test execution
import crypto from 'node:crypto';
import fs from 'node:fs';
import http from 'node:http';
import https from 'node:https';
import path from 'node:path';
import util from 'node:util';

import { OSRMDatastoreLoader, OSRMDirectLoader, OSRMmmapLoader } from '../lib/osrm_loader.js';

/** Global environment for all scenarios. */
class Env {
  // Initializes all environment constants and paths for test execution
  beforeAll(worldParameters) {
    const wp = this.wp = worldParameters;
    const penv = process.env;

    /** For scenarios that don't define a profile, osrm-extract still wants one. */
    this.DEFAULT_PROFILE = 'bicycle';
    /** Overrides any profile specified in a scenario. See: PR#4516 */
    this.OSRM_PROFILE = penv.OSRM_PROFILE;

    this.OSM_USER = 'osrm';
    this.OSM_UID = 1;
    this.OSM_TIMESTAMP = '2000-01-01T00:00:00Z';
    this.WAY_SPACING = 100;
    this.DEFAULT_GRID_SIZE = 100; // meters
    this.DEFAULT_ORIGIN = [1, 1];

    this.PLATFORM_CI = penv.GITHUB_ACTIONS != undefined;
    this.CUCUMBER_WORKER_ID = parseInt(penv.CUCUMBER_WORKER_ID || '0');

    // if (this.CUCUMBER_WORKER_ID == 0)
    //   console.log(wp);

    // make each worker use its own port
    wp.port += parseInt(this.CUCUMBER_WORKER_ID);
    wp.host = `http://${wp.ip}:${wp.port}`;

    if (wp.host.startsWith('https')) {
      this.client = https;
      this.agent = new https.Agent ({
        timeout: wp.httpTimeout,
        defaultPort: wp.port,
      });
    } else {
      this.client = http;
      this.agent = new http.Agent ({
        timeout: wp.httpTimeout,
        defaultPort: wp.port,
      });
    }

    this.DATASET_NAME = `cucumber${this.CUCUMBER_WORKER_ID}`;

    if (process.platform === 'win32') {
      this.TERMSIGNAL = 9;
      this.EXE = '.exe';
    } else {
      this.TERMSIGNAL = 'SIGTERM';
      this.EXE = '';
    }

    /**
     * A log file for the long running background process osrm-routed in load method
     * datastore. That process may output logs outside of a scenario.
     */
    this.globalLogfile = fs.openSync(
      path.join(wp.logsPath, `cucumber-global-${this.CUCUMBER_WORKER_ID}.log`),
      'a');

    // heuristically detect .so/.a/.dll/.lib suffix
    this.LIB = ['lib%s.a', 'lib%s.so', '%s.dll', '%s.lib'].find((format) => {
      try {
        const lib = path.join(wp.buildPath, util.format(format, 'osrm'));
        fs.accessSync(lib, fs.constants.F_OK);
      } catch {
        return false;
      }
      return true;
    });

    if (this.LIB === undefined) {
      throw new Error(
        '*** Unable to detect dynamic or static libosrm libraries',
      );
    }

    /** binaries responsible for the cached files */
    this.extractionBinaries = [];
    /** libraries responsible for the cached files */
    this.libraries = [];
    /** binaries that must be present */
    this.requiredBinaries = [];

    for (const i of ['extract', 'contract', 'customize', 'partition']) {
      const bin = path.join(wp.buildPath, `osrm-${i}${this.EXE}`);
      this.extractionBinaries.push(bin);
      this.requiredBinaries.push(bin);
    }
    for (const i of 'routed'.split()) {
      this.requiredBinaries.push(path.join(wp.buildPath, `osrm-${i}${this.EXE}`));
    }
    for (const i of ['_extract', '_contract', '_customize', '_partition', '']) {
      const lib = path.join(wp.buildPath, util.format(this.LIB, `osrm${i}`));
      this.libraries.push(lib);
    }

    if (!fs.existsSync(wp.testPath)) {
      callback(new Error(`*** Test folder doesn't exist: ${wp.testPath}`));
    };

    /** A hash of all osrm binaries and lua profiles */
    this.osrmHash = this.getOSRMHash();

    this.setLoadMethod(wp.loadMethod);
  }

  async afterAll() {
    await this.osrmLoader.afterAll();
    fs.closeSync(this.globalLogfile);
    this.globalLogfile = null;
  }

  globalLog(msg) {
    if (this.globalLogfile)
      fs.writeSync(this.globalLogfile, msg);
  }

  setLoadMethod(method) {
    if (method === 'datastore') {
      this.osrmLoader = new OSRMDatastoreLoader(this);
    } else if (method === 'directly') {
      this.osrmLoader = new OSRMDirectLoader(this);
    } else if (method === 'mmap') {
      this.osrmLoader = new OSRMmmapLoader(this);
    } else {
      this.osrmLoader = null;
      throw new Error(`No such data load method: ${method}`);
    }
  }

  getProfilePath(profile) {
    return path.join(wp.profilesPath, `${profile}.lua`);
  }

  // returns a hash of all OSRM code side dependencies
  // that is: all osrm binaries and all lua profiles
  getOSRMHash() {
    const dependencies = this.extractionBinaries.concat(this.libraries);

    const addLuaFiles = function (directory) {
      const luaFiles = fs.readdirSync(path.normalize(directory))
        .filter((f) => !!f.match(/\.lua$/))
        .map((f) => path.join(directory, f));
      Array.prototype.push.apply(dependencies, luaFiles);
    };

    addLuaFiles(this.wp.profilesPath);
    addLuaFiles(path.join(this.wp.profilesPath, 'lib'));

    if (this.OSRM_PROFILE) {
      // This may add a duplicate but it doesn't matter.
      dependencies.push(path.join(this.wp.profilesPath, `${this.OSRM_PROFILE}.lua`));
    }

    const checksum = crypto.createHash('md5');
    for (const file of dependencies) {
      checksum.update(fs.readFileSync(path.normalize(file)));
    }
    return checksum;
  }
}

/** Global environment */
export const env = new Env();
