// File content hashing utilities for cache invalidation and content verification
import fs from 'fs';
import crypto from 'crypto';
import d3 from 'd3-queue';

// Computes MD5 hash of multiple files concatenated together
const hashOfFiles = (paths, cb) => {
  const queue = d3.queue();
  for (let i = 0; i < paths.length; ++i) {
    queue.defer(fs.readFile, paths[i]);
  }
  queue.awaitAll((err, results) => {
    if (err) return cb(err);
    const checksum = crypto.createHash('md5');
    for (let i = 0; i < results.length; ++i) {
      checksum.update(results[i]);
    }
    cb(null, checksum.digest('hex'));
  });
};

// Computes MD5 hash of single file with optional additional content
const hashOfFile = (path, additional_content, cb) => {
  fs.readFile(path, (err, result) => {
    if (err) return cb(err);
    const checksum = crypto.createHash('md5');
    checksum.update(result + (additional_content || ''));
    cb(null, checksum.digest('hex'));
  });
};

export { hashOfFiles, hashOfFile };
