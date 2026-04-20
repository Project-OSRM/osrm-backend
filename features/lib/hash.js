// File content hashing utilities for cache invalidation and content verification
import fs from 'fs';
import crypto from 'crypto';

// Computes MD5 hash of multiple files concatenated together
const hashOfFiles = async (paths) => {
  const results = await Promise.all(paths.map(p => fs.promises.readFile(p)));
  const checksum = crypto.createHash('md5');
  results.forEach(r => checksum.update(r));
  return checksum.digest('hex');
};

// Computes MD5 hash of single file with optional additional content
const hashOfFile = async (path, additional_content) => {
  const result = await fs.promises.readFile(path);
  const checksum = crypto.createHash('md5');
  checksum.update(result);
  if (additional_content) {
    checksum.update(additional_content);
  }
  return checksum.digest('hex');
};

export { hashOfFiles, hashOfFile };
