var fs = require('fs');
var path = require('path');
var crypto = require('crypto');
var d3 = require('d3-queue');

module.exports = function () {
    this.hashOfFiles = (paths, cb) => {
        paths = Array.isArray(paths) ? paths : [paths];
        var shasum = crypto.createHash('sha1');

        var q = d3.queue(1);

        var addFile = (path, cb) => {
            fs.readFile(path, (err, data) => {
                shasum.update(data);
                cb(err);
            });
        };

        paths.forEach(path => { q.defer(addFile, path); });

        q.awaitAll(err => {
            if (err) throw new Error('*** Error reading files:', err);
            cb(shasum.digest('hex'));
        });
    };

    this.hashProfile = (cb) => {
        this.hashOfFiles(path.resolve(this.PROFILES_PATH, this.profile + '.lua'), cb);
    };

    this.hashString = (str) => {
        return crypto.createHash('sha1').update(str).digest('hex');
    };

    return this;
};
