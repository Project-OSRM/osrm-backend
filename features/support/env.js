// Sets up global environment constants and configuration for test execution
import path from 'path';
import util from 'util';
import fs from 'fs';
import d3 from 'd3-queue';
import child_process from 'child_process';
import tryConnect from '../lib/try_connect.js';
import { setDefaultTimeout } from '@cucumber/cucumber';

// Set global timeout for all steps and hooks
setDefaultTimeout(
  (process.env.CUCUMBER_TIMEOUT && parseInt(process.env.CUCUMBER_TIMEOUT)) ||
    5000,
);

// Sets up all constants that are valid for all features
export default class Env {
  constructor(world) {
    this.world = world;
  }

  // Initializes all environment constants and paths for test execution
  initializeEnv(callback) {
    this.TIMEOUT = parseInt(process.env.CUCUMBER_TIMEOUT) || 5000;
    this.ROOT_PATH = process.cwd();

    this.TEST_PATH = path.resolve(this.ROOT_PATH, 'test');
    this.CACHE_PATH = path.resolve(this.TEST_PATH, 'cache');
    this.LOGS_PATH = path.resolve(this.TEST_PATH, 'logs');

    this.PROFILES_PATH = path.resolve(this.ROOT_PATH, 'profiles');
    this.FIXTURES_PATH = path.resolve(this.ROOT_PATH, 'unit_tests/fixtures');
    this.BIN_PATH =
      process.env.OSRM_BUILD_DIR || path.resolve(this.ROOT_PATH, 'build');
    this.DATASET_NAME = 'cucumber';
    this.PLATFORM_WINDOWS = process.platform.match(/^win.*/);
    this.DEFAULT_ENVIRONMENT = process.env;
    this.DEFAULT_PROFILE = 'bicycle';
    this.DEFAULT_INPUT_FORMAT = 'osm';
    const loadMethod = process.env.OSRM_LOAD_METHOD || 'datastore';
    this.DEFAULT_LOAD_METHOD = loadMethod.match('mmap')
      ? 'mmap'
      : loadMethod.match('directly')
        ? 'directly'
        : 'datastore';
    this.DEFAULT_ORIGIN = [1, 1];
    this.OSM_USER = 'osrm';
    this.OSM_UID = 1;
    this.OSM_TIMESTAMP = '2000-01-01T00:00:00Z';
    this.WAY_SPACING = 100;
    this.DEFAULT_GRID_SIZE = 100; // meters
    // get algorithm name from the command line profile argument
    this.ROUTING_ALGORITHM = process.argv[process.argv.indexOf('-p') + 1].match(
      'mld',
    )
      ? 'MLD'
      : 'CH';
    this.TIMEZONE_NAMES = this.PLATFORM_WINDOWS ? 'win' : 'iana';

    this.OSRM_PORT = parseInt(process.env.OSRM_PORT) || 5000;
    this.OSRM_IP = process.env.OSRM_IP || '127.0.0.1';
    this.OSRM_CONNECTION_RETRIES =
      parseInt(process.env.OSRM_CONNECTION_RETRIES) || 10;
    this.OSRM_CONNECTION_EXP_BACKOFF_COEF =
      parseFloat(process.env.OSRM_CONNECTION_EXP_BACKOFF_COEF) || 1.1;

    this.HOST = `http://${this.OSRM_IP}:${this.OSRM_PORT}`;

    this.OSRM_PROFILE = process.env.OSRM_PROFILE;

    if (this.PLATFORM_WINDOWS) {
      this.TERMSIGNAL = 9;
      this.EXE = '.exe';
    } else {
      this.TERMSIGNAL = 'SIGTERM';
      this.EXE = '';
    }

    // heuristically detect .so/.a/.dll/.lib suffix
    this.LIB = ['lib%s.a', 'lib%s.so', '%s.dll', '%s.lib'].find((format) => {
      try {
        const lib = `${this.BIN_PATH}/${util.format(format, 'osrm')}`;
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

    this.OSRM_EXTRACT_PATH = path.resolve(
      util.format('%s/%s%s', this.BIN_PATH, 'osrm-extract', this.EXE),
    );
    this.OSRM_CONTRACT_PATH = path.resolve(
      util.format('%s/%s%s', this.BIN_PATH, 'osrm-contract', this.EXE),
    );
    this.OSRM_CUSTOMIZE_PATH = path.resolve(
      util.format('%s/%s%s', this.BIN_PATH, 'osrm-customize', this.EXE),
    );
    this.OSRM_PARTITION_PATH = path.resolve(
      util.format('%s/%s%s', this.BIN_PATH, 'osrm-partition', this.EXE),
    );
    this.OSRM_ROUTED_PATH = path.resolve(
      util.format('%s/%s%s', this.BIN_PATH, 'osrm-routed', this.EXE),
    );
    ((this.LIB_OSRM_EXTRACT_PATH = util.format(
      `%s/${this.LIB}`,
      this.BIN_PATH,
      'osrm_extract',
    )),
    (this.LIB_OSRM_CONTRACT_PATH = util.format(
      `%s/${this.LIB}`,
      this.BIN_PATH,
      'osrm_contract',
    )),
    (this.LIB_OSRM_CUSTOMIZE_PATH = util.format(
      `%s/${this.LIB}`,
      this.BIN_PATH,
      'osrm_customize',
    )),
    (this.LIB_OSRM_PARTITION_PATH = util.format(
      `%s/${this.LIB}`,
      this.BIN_PATH,
      'osrm_partition',
    )),
    (this.LIB_OSRM_PATH = util.format(
      `%s/${this.LIB}`,
      this.BIN_PATH,
      'osrm',
    )));

    fs.exists(this.TEST_PATH, (exists) => {
      if (exists) return callback();
      else return callback(new Error('*** Test folder doesn\'t exist.'));
    });
  }

  getProfilePath(profile) {
    return path.resolve(this.PROFILES_PATH, `${profile}.lua`);
  }

  verifyOSRMIsNotRunning(callback) {
    tryConnect(this.OSRM_IP, this.OSRM_PORT, (err) => {
      if (!err)
        return callback(new Error('*** osrm-routed is already running.'));
      else callback();
    });
  }

  verifyExistenceOfBinaries(callback) {
    const verify = function (binPath, cb) {
      fs.exists(binPath, (exists) => {
        if (!exists)
          return cb(
            new Error(util.format('%s is missing. Build failed?', binPath)),
          );
        const helpPath = util.format('%s --help', binPath);
        child_process.exec(helpPath, (err) => {
          if (err) {
            return cb(
              new Error(
                util.format('*** %s exited with code %d', helpPath, err.code),
              ),
            );
          }
          cb();
        });
      });
    };

    const q = d3.queue();
    [
      this.OSRM_EXTRACT_PATH,
      this.OSRM_CONTRACT_PATH,
      this.OSRM_CUSTOMIZE_PATH,
      this.OSRM_PARTITION_PATH,
      this.OSRM_ROUTED_PATH,
    ].forEach((bin) => {
      q.defer(verify, bin);
    });
    q.awaitAll(callback);
  }
}
