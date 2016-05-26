var path = require('path');
var util = require('util');
var fs = require('fs');
var exec = require('child_process').exec;
var d3 = require('d3-queue');

module.exports = function () {
    this.initializeEnv = (callback) => {
        this.OSRM_PORT = process.env.OSRM_PORT && parseInt(process.env.OSRM_PORT) || 5000;
        this.TIMEOUT = process.env.CUCUMBER_TIMEOUT && parseInt(process.env.CUCUMBER_TIMEOUT) || 5000;
        this.setDefaultTimeout(this.TIMEOUT);
        this.ROOT_FOLDER = process.cwd();
        this.OSM_USER = 'osrm';
        this.OSM_GENERATOR = 'osrm-test';
        this.OSM_UID = 1;
        this.TEST_FOLDER = path.resolve(this.ROOT_FOLDER, 'test');
        this.DATA_FOLDER = path.resolve(this.TEST_FOLDER, 'cache');
        this.OSM_TIMESTAMP = '2000-01-01T00:00:00Z';
        this.DEFAULT_SPEEDPROFILE = 'bicycle';
        this.WAY_SPACING = 100;
        this.DEFAULT_GRID_SIZE = 100;    // meters
        this.PROFILES_PATH = path.resolve(this.ROOT_FOLDER, 'profiles');
        this.FIXTURES_PATH = path.resolve(this.ROOT_FOLDER, 'unit_tests/fixtures');
        this.BIN_PATH = path.resolve(this.ROOT_FOLDER, 'build');
        this.DEFAULT_INPUT_FORMAT = 'osm';
        this.DEFAULT_ORIGIN = [1,1];
        this.DEFAULT_LOAD_METHOD = 'datastore';
        this.OSRM_ROUTED_LOG_FILE = path.resolve(this.TEST_FOLDER, 'osrm-routed.log');
        this.ERROR_LOG_FILE = path.resolve(this.TEST_FOLDER, 'error.log');

        // OS X shim to ensure shared libraries from custom locations can be loaded
        // This is needed in OS X >= 10.11 because DYLD_LIBRARY_PATH is blocked
        // https://forums.developer.apple.com/thread/9233
        this.LOAD_LIBRARIES = process.env.OSRM_SHARED_LIBRARY_PATH ? util.format('DYLD_LIBRARY_PATH=%s ', process.env.OSRM_SHARED_LIBRARY_PATH) : '';

        // TODO make sure this works on win
        if (process.platform.match(/indows.*/)) {
            this.TERMSIGNAL = 9;
            this.EXE = '.exe';
            this.QQ = '"';
        } else {
            this.TERMSIGNAL = 'SIGTERM';
            this.EXE = '';
            this.QQ = '';
        }

        // eslint-disable-next-line no-console
        console.info(util.format('Node Version', process.version));
        if (parseInt(process.version.match(/v(\d)/)[1]) < 4) throw new Error('*** PLease upgrade to Node 4.+ to run OSRM cucumber tests');

        fs.exists(this.TEST_FOLDER, (exists) => {
            if (!exists) throw new Error(util.format('*** Test folder %s doesn\'t exist.', this.TEST_FOLDER));
            callback();
        });
    };

    this.verifyOSRMIsNotRunning = () => {
        if (this.OSRMLoader.up()) {
            throw new Error('*** osrm-routed is already running.');
        }
    };

    this.verifyExistenceOfBinaries = (callback) => {
        var verify = (bin, cb) => {
            var binPath = path.resolve(util.format('%s/%s%s', this.BIN_PATH, bin, this.EXE));
            fs.exists(binPath, (exists) => {
                if (!exists) throw new Error(util.format('%s is missing. Build failed?', binPath));
                var helpPath = util.format('%s%s --help > /dev/null 2>&1', this.LOAD_LIBRARIES, binPath);
                exec(helpPath, (err) => {
                    if (err) {
                        this.log(util.format('*** Exited with code %d', err.code), 'preprocess');
                        throw new Error(util.format('*** %s exited with code %d', helpPath, err.code));
                    }
                    cb();
                });
            });
        };

        var q = d3.queue();
        ['osrm-extract', 'osrm-contract', 'osrm-routed'].forEach(bin => { q.defer(verify, bin); });
        q.awaitAll(() => {
            callback();
        });
    };

    this.AfterConfiguration = (callback) => {
        this.clearLogFiles(() => {
            this.verifyOSRMIsNotRunning();
            this.verifyExistenceOfBinaries(() => {
                callback();
            });
        });
    };

    process.on('exit', () => {
        if (this.OSRMLoader.loader) this.OSRMLoader.shutdown(() => {});
    });

    process.on('SIGINT', () => {
        process.exit(2);
        // TODO need to handle for windows??
    });
};
