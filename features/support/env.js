'use strict';

const path = require('path');
const util = require('util');
const fs = require('fs');
const d3 = require('d3-queue');
const child_process = require('child_process');
const tryConnect = require('../lib/try_connect');

// Sets up all constants that are valid for all features
module.exports = function () {
    this.initializeEnv = (callback) => {
        this.TIMEOUT = process.env.CUCUMBER_TIMEOUT && parseInt(process.env.CUCUMBER_TIMEOUT) || 5000;
        // set cucumber default timeout
        this.setDefaultTimeout(this.TIMEOUT);
        this.ROOT_PATH = process.cwd();

        this.TEST_PATH = path.resolve(this.ROOT_PATH, 'test');
        this.CACHE_PATH = path.resolve(this.TEST_PATH, 'cache');
        this.LOGS_PATH = path.resolve(this.TEST_PATH, 'logs');

        this.PROFILES_PATH = path.resolve(this.ROOT_PATH, 'profiles');
        this.FIXTURES_PATH = path.resolve(this.ROOT_PATH, 'unit_tests/fixtures');
        this.BIN_PATH = process.env.OSRM_BUILD_DIR && process.env.OSRM_BUILD_DIR || path.resolve(this.ROOT_PATH, 'build');
        var stxxl_config = path.resolve(this.ROOT_PATH, 'test/.stxxl');
        if (!fs.existsSync(stxxl_config)) {
            return callback(new Error('*** '+stxxl_config+ 'does not exist'));
        }

        this.DATASET_NAME = 'cucumber';
        this.PLATFORM_WINDOWS = process.platform.match(/^win.*/);
        this.DEFAULT_ENVIRONMENT = Object.assign({STXXLCFG: stxxl_config}, process.env);
        this.DEFAULT_PROFILE = 'bicycle';
        this.DEFAULT_INPUT_FORMAT = 'osm';
        this.DEFAULT_LOAD_METHOD = process.argv[process.argv.indexOf('-m') +1].match('mmap') ? 'mmap' : 'datastore';
        this.DEFAULT_ORIGIN = [1,1];
        this.OSM_USER = 'osrm';
        this.OSM_UID = 1;
        this.OSM_TIMESTAMP = '2000-01-01T00:00:00Z';
        this.WAY_SPACING = 100;
        this.DEFAULT_GRID_SIZE = 100; // meters
        // get algorithm name from the command line profile argument
        this.ROUTING_ALGORITHM = process.argv[process.argv.indexOf('-p') + 1].match('mld') ? 'MLD' : 'CH';
        this.TIMEZONE_NAMES = this.PLATFORM_WINDOWS ? 'win' : 'iana';

        this.OSRM_PORT = process.env.OSRM_PORT && parseInt(process.env.OSRM_PORT) || 5000;
        this.OSRM_IP = process.env.OSRM_IP || '127.0.0.1';
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
                const lib = this.BIN_PATH + '/' + util.format(format, 'osrm');
                fs.accessSync(lib, fs.F_OK);
            } catch(e) { return false; }
            return true;
        });

        if (this.LIB === undefined) {
            throw new Error('*** Unable to detect dynamic or static libosrm libraries');
        }

        this.OSRM_EXTRACT_PATH = path.resolve(util.format('%s/%s%s', this.BIN_PATH, 'osrm-extract', this.EXE));
        this.OSRM_CONTRACT_PATH = path.resolve(util.format('%s/%s%s', this.BIN_PATH, 'osrm-contract', this.EXE));
        this.OSRM_ROUTED_PATH = path.resolve(util.format('%s/%s%s', this.BIN_PATH, 'osrm-routed', this.EXE));
        this.LIB_OSRM_EXTRACT_PATH = util.format('%s/' + this.LIB, this.BIN_PATH, 'osrm_extract'),
        this.LIB_OSRM_GUIDANCE_PATH = util.format('%s/' + this.LIB, this.BIN_PATH, 'osrm_guidance'),
        this.LIB_OSRM_CONTRACT_PATH = util.format('%s/' + this.LIB, this.BIN_PATH, 'osrm_contract'),
        this.LIB_OSRM_PATH = util.format('%s/' + this.LIB, this.BIN_PATH, 'osrm');

        // eslint-disable-next-line no-console
        console.info(util.format('Node Version', process.version));
        if (parseInt(process.version.match(/v(\d+)/)[1]) < 4) throw new Error('*** Please upgrade to Node 4.+ to run OSRM cucumber tests');

        fs.exists(this.TEST_PATH, (exists) => {
            if (exists)
                return callback();
            else
                return callback(new Error('*** Test folder doesn\'t exist.'));
        });
    };

    this.getProfilePath = (profile) => {
        return path.resolve(this.PROFILES_PATH, profile + '.lua');
    };

    this.verifyOSRMIsNotRunning = (callback) => {
        tryConnect(this.OSRM_IP, this.OSRM_PORT, (err) => {
            if (!err) return callback(new Error('*** osrm-routed is already running.'));
            else callback();
        });
    };

    this.verifyExistenceOfBinaries = (callback) => {
        var verify = (binPath, cb) => {
            fs.exists(binPath, (exists) => {
                if (!exists) return cb(new Error(util.format('%s is missing. Build failed?', binPath)));
                var helpPath = util.format('%s --help', binPath);
                child_process.exec(helpPath, (err) => {
                    if (err) {
                        return cb(new Error(util.format('*** %s exited with code %d', helpPath, err.code)));
                    }
                    cb();
                });
            });
        };

        var q = d3.queue();
        [this.OSRM_EXTRACT_PATH, this.OSRM_CONTRACT_PATH, this.OSRM_ROUTED_PATH].forEach(bin => { q.defer(verify, bin); });
        q.awaitAll(callback);
    };

    process.on('exit', () => {
        this.osrmLoader.shutdown(() => {});
    });

    process.on('SIGINT', () => {
        process.exit(2);
        // TODO need to handle for windows??
    });
};
