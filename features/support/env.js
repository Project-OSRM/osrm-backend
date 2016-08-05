var path = require('path');
var util = require('util');
var fs = require('fs');
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
        this.BIN_PATH = process.env.OSRM_BUILD_DIR && process.env.OSRM_BUILD_DIR || path.resolve(this.ROOT_FOLDER, 'build');
        this.DEFAULT_INPUT_FORMAT = 'osm';
        this.DEFAULT_ORIGIN = [1,1];
        this.DEFAULT_LOAD_METHOD = 'datastore';
        this.OSRM_ROUTED_LOG_FILE = path.resolve(this.TEST_FOLDER, 'osrm-routed.log');
        this.ERROR_LOG_FILE = path.resolve(this.TEST_FOLDER, 'error.log');

        // TODO make sure this works on win
        if (process.platform.match(/indows.*/)) {
            this.TERMSIGNAL = 9;
            this.EXE = '.exe';
            this.LIB = '.dll';
            this.QQ = '"';
        } else {
            this.TERMSIGNAL = 'SIGTERM';
            this.EXE = '';
            this.LIB = '.so';
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

    process.on('exit', () => {
        if (this.OSRMLoader.loader) this.OSRMLoader.shutdown(() => {});
    });

    process.on('SIGINT', () => {
        process.exit(2);
        // TODO need to handle for windows??
    });
};
