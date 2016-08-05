var fs = require('fs');
var path = require('path');
var util = require('util');
var d3 = require('d3-queue');
var OSM = require('./build_osm');
var classes = require('./data_classes');
var child_process = require('child_process');

module.exports = function () {
    this.initializeOptions = (callback) => {
        this.profile = this.profile || this.DEFAULT_SPEEDPROFILE;

        this.OSMDB = this.OSMDB || new OSM.DB();

        this.nameNodeHash = this.nameNodeHash || {};

        this.locationHash = this.locationHash || {};

        this.nameWayHash = this.nameWayHash || {};

        this.osmData = new classes.osmData(this);

        this.OSRMLoader = this._OSRMLoader();

        this.PREPROCESS_LOG_FILE = path.resolve(this.TEST_FOLDER, 'preprocessing.log');

        this.LOG_FILE = path.resolve(this.TEST_FOLDER, 'fail.log');

        this.HOST = 'http://127.0.0.1:' + this.OSRM_PORT;

        this.DESTINATION_REACHED = 15;  // OSRM instruction code

        this.shortcutsHash = this.shortcutsHash || {};

        var hashLuaLib = (cb) => {
            fs.readdir(path.normalize(this.PROFILES_PATH + '/lib/'), (err, files) => {
                if (err) cb(err);
                var luaFiles = files.filter(f => !!f.match(/\.lua$/)).map(f => path.normalize(this.PROFILES_PATH + '/lib/' + f));
                this.hashOfFiles(luaFiles, hash => {
                    this.luaLibHash = hash;
                    cb();
                });
            });
        };

        var hashProfile = (cb) => {
            this.hashProfile((hash) => {
                this.profileHash = hash;
                cb();
            });
        };

        var hashExtract = (cb) => {
            var files = [ util.format('%s/osrm-extract%s', this.BIN_PATH, this.EXE),
                          util.format('%s/libosrm_extract%s', this.BIN_PATH, this.LIB) ];
            this.hashOfFiles(files, (hash) => {
                this.binExtractHash = hash;
                cb();
            });
        };

        var hashContract = (cb) => {
            var files = [ util.format('%s/osrm-contract%s', this.BIN_PATH, this.EXE),
                          util.format('%s/libosrm_contract%s', this.BIN_PATH, this.LIB) ];
            this.hashOfFiles(files, (hash) => {
                this.binContractHash = hash;
                cb();
            });
        };

        var hashRouted = (cb) => {
            var files = [ util.format('%s/osrm-routed%s', this.BIN_PATH, this.EXE),
                          util.format('%s/libosrm%s', this.BIN_PATH, this.LIB) ];
            this.hashOfFiles(files, (hash) => {
                this.binRoutedHash = hash;
                this.fingerprintRoute = this.hashString(this.binRoutedHash);
                cb();
            });
        };

        d3.queue()
            .defer(hashLuaLib)
            .defer(hashProfile)
            .defer(hashExtract)
            .defer(hashContract)
            .defer(hashRouted)
            .awaitAll(() => {
                this.clearLogFiles(() => {
                    this.verifyOSRMIsNotRunning();
                    this.verifyExistenceOfBinaries(() => {
                        callback();
                    });
                });
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
                var helpPath = util.format('%s --help > /dev/null 2>&1', binPath);
                child_process.exec(helpPath, (err) => {
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

    this.updateFingerprintExtract = (str) => {
        this.fingerprintExtract = this.hashString([this.fingerprintExtract, str].join('-'));
    };

    this.updateFingerprintContract = (str) => {
        this.fingerprintContract = this.hashString([this.fingerprintContract, str].join('-'));
    };

    this.setProfile = (profile, cb) => {
        var lastProfile = this.profile;
        if (profile !== lastProfile) {
            this.profile = profile;
            this.hashProfile((hash) => {
                this.profileHash = hash;
                this.updateFingerprintExtract(this.profileHash);
                cb();
            });
        } else {
            this.updateFingerprintExtract(this.profileHash);
            cb();
        }
    };

    this.setExtractArgs = (args, callback) => {
        this.extractArgs = args;
        this.updateFingerprintExtract(args);
        callback();
    };

    this.setContractArgs = (args, callback) => {
        this.contractArgs = args;
        this.updateFingerprintContract(args);
        callback();
    };
};
