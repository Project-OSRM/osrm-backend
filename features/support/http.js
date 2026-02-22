// HTTP client utilities for making API requests to OSRM routing server
import { env } from './env.js';

export function sendRequest (url, log, callback) {
  log(`sending request: ${url}`);
  const req = env.client.get (url, { agent: env.agent }, (res) => {
    let data = '';

    // Collect data chunks
    res.on('data', (chunk) => {
      data += chunk;
    });

    // Handle end of response
    res.on('end', () => {
      callback(null, res, data);
    });
  });

  // Handle timeout
  req.on('timeout', (err) => {
    log(`request timed out: ${url}`);
    req.destroy();
    callback(err);
  });

  // Handle errors
  req.on('error', (err) => {
    log(`request errored out: ${url} ${err.message}`);
    callback(err);
  });

  req.end();
};
