'use strict';

const fs = require('fs');
const crypto = require('crypto');
const d3 = require('d3-queue');

module.exports =  {
    hashOfFiles: (paths, cb) => {
        let queue = d3.queue();
        for (let i = 0; i < paths.length; ++i) {
            queue.defer(fs.readFile, paths[i]);
        }
        queue.awaitAll((err, results) => {
            if (err) return cb(err);
            let checksum = crypto.createHash('md5');
            for (let i = 0; i < results.length; ++i) {
                checksum.update(results[i]);
            }
            cb(null, checksum.digest('hex'));
        });
    },

     hashOfFile: (path, additional_content, cb) => {
        fs.readFile(path, (err, result) => {
            if (err) return cb(err);
            let checksum = crypto.createHash('md5');
            checksum.update(result + (additional_content || "") );
            cb(null, checksum.digest('hex'));
        });
    }
};
