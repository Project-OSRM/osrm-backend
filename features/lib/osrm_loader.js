'use strict';

var fs = require('fs');
var util = require('util');
var net = require('net');
var Timeout = require('node-timeout');

class OSRMBaseLoader{
    constructor (scope) {
        this.scope = scope;
        this.child = null;
    }

    launch (callback) {
        var limit = Timeout(this.scope.TIMEOUT, { err: new Error('*** Launching osrm-routed timed out.') });

        var runLaunch = (cb) => {
            this.osrmUp(() => { this.waitForConnection(cb); });
        };

        runLaunch(limit((e) => { if (e) callback(e); else callback(); }));
    }

    shutdown (callback) {
        var limit = Timeout(this.scope.TIMEOUT, { err: new Error('*** Shutting down osrm-routed timed out.')});

        var runShutdown = (cb) => {
            this.osrmDown(cb);
        };

        runShutdown(limit((e) => { if (e) callback(e); else callback(); }));
    }

    osrmIsRunning () {
        return this.child && !this.child.killed;
    }

    osrmDown (callback) {
        if (this.osrmIsRunning()) {
            process.kill(this.child.pid, this.scope.TERMSIGNAL);
            this.waitForShutdown(callback);
        } else callback();
    }

    waitForConnection (callback) {
        var retryCount = 0;
        var connectWithRetry = () => {
            net.connect({ port: this.scope.OSRM_PORT, host: '127.0.0.1' })
               .on('connect', () => { callback(); })
               .on('error', () => {
                   if (retryCount < 2) {
                       retryCount++;
                       setTimeout(connectWithRetry, 100);
                   } else {
                       callback(new Error('Could not connect to osrm-routed after three retires'));
                   }
               });
        };

        connectWithRetry();
    }

    waitForShutdown (callback) {
        var check = () => {
            if (!this.osrmIsRunning()) return callback();
        };
        setTimeout(check, 100);
    }
};

class OSRMDirectLoader extends OSRMBaseLoader {
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
        if (this.osrmIsRunning()) return callback();

        this.child = this.scope.runBin('osrm-routed', util.format("%s -p %d", this.inputFile, this.scope.OSRM_PORT));
        callback();
    }
};

class OSRMDatastoreLoader extends OSRMBaseLoader {
    constructor (scope) {
        super(scope);
    }

    load (inputFile, callback) {
        this.inputFile = inputFile;

        this.loadData((err) => {
            if (err) return callback(err);
            if (this.osrmIsRunning()) return this.launch(callback);
            else callback();
        });
    }

    loadData (callback) {
        this.scope.runBin('osrm-datastore', this.inputFile, (err) => {
            if (err) return callback(new Error('*** osrm-datastore exited with ' + err.code + ': ' + err));
            callback();
        });
    }

    osrmUp (callback) {
        if (this.osrmIsRunning()) return callback();

        this.child = this.scope.runBin('osrm-routed', util.format('--shared-memory=1 -p %d', this.scope.OSRM_PORT));

        callback();
    }
};

class OSRMLoader {
    constructor (scope) {
        this.scope = scope;
        this.loader = null;
    }

    load (inputFile, callback) {
        let method = this.scope.loadMethod;
        if (method === 'datastore') {
            this.loader = new OSRMDatastoreLoader(this.scope);
            this.loader.load(inputFile, callback);
        } else if (method === 'directly') {
            this.loader = new OSRMDirectLoader(this.scope);
            this.loader.load(inputFile, callback);
        } else {
            callback(new Error('*** Unknown load method ' + method));
        }
    }

    shutdown (callback) {
        if (!this.loader) return callback();

        this.loader.shutdown(callback);
    }

    up () {
        return this.loader ? this.loader.osrmIsRunning() : false;
    }
};

module.exports = OSRMLoader;
