var fs = require('fs');

module.exports = function () {
    this.clearLogFiles = (callback) => {
        // emptying existing files, rather than deleting and writing new ones makes it
        // easier to use tail -f from the command line
        fs.writeFile(this.OSRM_ROUTED_LOG_FILE, '', err => {
            if (err) throw err;
            fs.writeFile(this.PREPROCESS_LOG_FILE, '', err => {
                if (err) throw err;
                fs.writeFile(this.LOG_FILE, '', err => {
                    if (err) throw err;
                    callback();
                });
            });
        });
    };

    var log = this.log = (s, type) => {
        s = s || '';
        type = type || null;
        var file = type === 'preprocess' ? this.PREPROCESS_LOG_FILE : this.LOG_FILE;
        fs.appendFile(file, s + '\n', err => {
            if (err) throw err;
        });
    };

    this.logScenarioFailInfo = () => {
        if (this.hasLoggedScenarioInfo) return;

        log('=========================================');
        log('Failed scenario: ' + this.scenarioTitle);
        log('Time: ' + this.scenarioTime);
        log('Fingerprint osm stage: ' + this.osmData.fingerprintOSM);
        log('Fingerprint extract stage: ' + this.fingerprintExtract);
        log('Fingerprint contract stage: ' + this.fingerprintContract);
        log('Fingerprint route stage: ' + this.fingerprintRoute);
        log('Profile: ' + this.profile);
        log();
        log('```xml');               // so output can be posted directly to github comment fields
        log(this.osmData.str.trim());
        log('```');
        log();
        log();

        this.hasLoggedScenarioInfo = true;
    };

    this.logFail = (expected, got, attempts) => {
        this.logScenarioFailInfo();
        log('== ');
        log('Expected: ' + JSON.stringify(expected));
        log('Got:      ' + JSON.stringify(got));
        log();
        ['route','forw','backw'].forEach((direction) => {
            if (attempts[direction]) {
                log('Direction: ' + direction);
                log('Query: ' + attempts[direction].query);
                log('Response: ' + attempts[direction].response.body);
                log();
            }
        });
    };

    this.logPreprocessInfo = () => {
        if (this.hasLoggedPreprocessInfo) return;
        log('=========================================', 'preprocess');
        log('Preprocessing data for scenario: ' + this.scenarioTitle, 'preprocess');
        log('Time: ' + this.scenarioTime, 'preprocess');
        log('', 'preprocess');
        log('== OSM data:', 'preprocess');
        log('```xml', 'preprocess');            // so output can be posted directly to github comment fields
        log(this.osmData.str, 'preprocess');
        log('```', 'preprocess');
        log('', 'preprocess');
        log('== Profile:', 'preprocess');
        log(this.profile, 'preprocess');
        log('', 'preprocess');
        this.hasLoggedPreprocessInfo = true;
    };

    this.logPreprocess = (str) => {
        this.logPreprocessInfo();
        log(str, 'preprocess');
    };

    this.logPreprocessDone = () => {
        log('Done with preprocessing at ' + new Date(), 'preprocess');
    };
};
