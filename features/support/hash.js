var fs = require('fs');
var path = require('path');
var crypto = require('crypto');
var d3 = require('d3-queue');

module.exports = function () {
    this.hashOfFiles = (paths, cb) => {
        paths = Array.isArray(paths) ? paths : [paths];
        var shasum = crypto.createHash('sha1'), hashedFiles = false;

        var q = d3.queue(1);

        var addFile = (path, cb) => {
            fs.readFile(path, (err, data) => {
                if (err && err.code === 'ENOENT') cb(); // ignore non-existing files
                else if (err) cb(err);
                else {
                    shasum.update(data);
                    hashedFiles = true;
                    cb();
                }
            });
        };

        paths.forEach(path => { q.defer(addFile, path); });

        q.awaitAll(err => {
            if (err) throw new Error('*** Error reading files:', err);
            if (!hashedFiles) throw new Error('*** No files found: [' + paths.join(', ') + ']');
            cb(shasum.digest('hex'));
        });
    };

    this.hashString = (str) => {
        return crypto.createHash('md5').update(str).digest('hex');
    };

    return this;
};
