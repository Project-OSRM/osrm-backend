// Network connectivity testing utility for checking if OSRM server is ready
import net from 'net';

// Attempts TCP connection to test if server is accepting connections
export default function tryConnect(host, port, callback) {
  net
    .connect({ port, host })
    .on('connect', () => {
      callback();
    })
    .on('error', () => {
      callback(new Error('Could not connect.'));
    });
}
