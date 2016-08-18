var fs = require('fs');

module.exports = function () {
    this.clearLogFiles = (callback) => {
        // emptying existing files, rather than deleting and writing new ones makes it
        // easier to use tail -f from the command line
        fs.writeFile(this.OSRM_ROUTED_LOG_FILE, '', err => {
            if (err) throw err;
            fs.writeFile(this.LOG_FILE, '', err => {
                if (err) throw err;
                callback();
            });
        });
    };

    this.log = (message) => {
        fs.appendFile(this.LOG_FILE, s + '\n', err => {
            if (err) throw err;
        });
    };
};
