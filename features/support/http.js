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

// Sends a POST request with a JSON body (used to exercise the JSON POST API).
export function sendPostRequest (url, body, log, callback) {
  const payload = typeof body === 'string' ? body : JSON.stringify(body);
  log(`sending POST request: ${url} body: ${payload}`);
  const req = env.client.request(
    url,
    {
      agent: env.agent,
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(payload),
      },
    },
    (res) => {
      let data = '';
      res.on('data', (chunk) => {
        data += chunk;
      });
      res.on('end', () => {
        callback(null, res, data);
      });
    });

  req.on('timeout', (err) => {
    log(`request timed out: ${url}`);
    req.destroy();
    callback(err);
  });

  req.on('error', (err) => {
    log(`request errored out: ${url} ${err.message}`);
    callback(err);
  });

  req.write(payload);
  req.end();
};
