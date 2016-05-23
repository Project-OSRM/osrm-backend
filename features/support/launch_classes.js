'use strict';

var fs = require('fs');
var spawn = require('child_process').spawn;
var util = require('util');
var Timeout = require('node-timeout');

var OSRMBaseLoader = class {
    constructor (scope) {
        this.scope = scope;
    }

    launch (callback) {
        var limit = Timeout(this.scope.TIMEOUT, { err: this.scope.RoutedError('Launching osrm-routed timed out.') });

        var runLaunch = (cb) => {
            this.osrmUp(cb);
        };

        runLaunch(limit((e) => { if (e) callback(e); else callback(); }));
    }

    shutdown (callback) {
        var limit = Timeout(this.scope.TIMEOUT, { err: this.scope.RoutedError('Shutting down osrm-routed timed out.')});

        var runShutdown = (cb) => {
            this.osrmDown(cb);
        };

        runShutdown(limit((e) => { if (e) callback(e); else callback(); }));
    }

    osrmIsRunning () {
        return !!this.scope.pid && this.child && !this.child.killed;
    }

    osrmDown (callback) {
        if (this.scope.pid) {
            process.kill(this.scope.pid, this.scope.TERMSIGNAL);
            this.waitForShutdown(callback);
            this.scope.pid = null;
        } else callback(true);
    }

    waitForShutdown (callback) {
        var check = () => {
            if (!this.osrmIsRunning()) return callback();
        };
        setTimeout(check, 100);
    }
};

var OSRMDirectLoader = class extends OSRMBaseLoader {
    constructor (scope) {
        super(scope);
    }

    load (inputFile, callback) {
        this.inputFile = inputFile;
        this.shutdown(() => {
            this.launch(callback);
        });
    }

    osrmUp (callback) {
        if (this.scope.pid) return callback();
        var writeToLog = (data) => {
            fs.appendFile(this.scope.OSRM_ROUTED_LOG_FILE, data, (err) => { if (err) throw err; });
        };

        var child = spawn(util.format('%s%s/osrm-routed', this.scope.LOAD_LIBRARIES, this.scope.BIN_PATH), [this.inputFile, util.format('-p%d', this.scope.OSRM_PORT)]);
        this.scope.pid = child.pid;
        child.stdout.on('data', writeToLog);
        child.stderr.on('data', writeToLog);

        callback();
    }
};

var OSRMDatastoreLoader = class extends OSRMBaseLoader {
    constructor (scope) {
        super(scope);
    }

    load (inputFile, callback) {
        this.inputFile = inputFile;
        this.loadData((err) => {
            if (err) return callback(err);
            if (!this.scope.pid) return this.launch(callback);
            else callback();
        });
    }

    loadData (callback) {
        this.scope.runBin('osrm-datastore', this.inputFile, (err) => {
            if (err) return callback(this.scope.LaunchError(this.exitCode, 'datastore', err));
            callback();
        });
    }

    osrmUp (callback) {
        if (this.scope.pid) return callback();
        var writeToLog = (data) => {
            fs.appendFile(this.scope.OSRM_ROUTED_LOG_FILE, data, (err) => { if (err) throw err; });
        };

        var child = spawn(util.format('%s%s/osrm-routed', this.scope.LOAD_LIBRARIES, this.scope.BIN_PATH), ['--shared-memory=1', util.format('-p%d', this.scope.OSRM_PORT)]);
        this.child = child;
        this.scope.pid = child.pid;
        child.stdout.on('data', writeToLog);
        child.stderr.on('data', writeToLog);

        callback();
    }
};

module.exports = {
    _OSRMLoader: class {
        constructor (scope) {
            this.scope = scope;
            this.loader = null;
        }

        load (inputFile, callback) {
            var method = this.scope.loadMethod;
            if (method === 'datastore') {
                this.loader = new OSRMDatastoreLoader(this.scope);
                this.loader.load(inputFile, callback);
            } else if (method === 'directly') {
                this.loader = new OSRMDirectLoader(this.scope);
                this.loader.load(inputFile, callback);
            } else {
                throw new Error('*** Unknown load method ' + method);
            }
        }

        shutdown (callback) {
            this.loader.shutdown(callback);
        }

        up () {
            return this.loader ? this.loader.osrmIsRunning() : false;
        }
    }
};
