// Process execution utilities for running OSRM binaries and managing subprocesses
import path from 'path';
import fs from 'fs';
import util from 'util';
import child_process from 'child_process';

export default class Run {
  constructor(world) {
    this.world = world;
  }

  // replaces placeholders for in user supplied commands
  expandOptions(options) {
    let opts = options.slice();
    let table = {
      '{osm_file}': this.inputCacheFile,
      '{processed_file}': this.processedCacheFile,
      '{profile_file}': this.profileFile,
      '{rastersource_file}': this.rasterCacheFile,
      '{speeds_file}': this.speedsCacheFile,
      '{penalties_file}': this.penaltiesCacheFile,
      '{timezone_names}': this.TIMEZONE_NAMES
    };

    for (let k in table) {
      opts = opts.replace(k, table[k]);
    }

    return opts;
  }

  setupOutputLog(process, log) {
    if (process.logFunc) {
      process.stdout.removeListener('data', process.logFunc);
      process.stderr.removeListener('data', process.logFunc);
    }

    process.logFunc = (message) => { log.write(message); };
    process.stdout.on('data', process.logFunc);
    process.stderr.on('data', process.logFunc);
  }

  runBin(bin, options, env, callback) {
    let cmd = path.resolve(util.format('%s/%s%s', this.BIN_PATH, bin, this.EXE));
    let opts = options.split(' ').filter((x) => { return x && x.length > 0; });
    let log = fs.createWriteStream(this.scenarioLogFile, {'flags': 'a'});
    log.write(util.format('*** running %s %s\n', cmd, options));
    
    // we need to set a large maxbuffer here because we have long running processes like osrm-routed
    // with lots of log output
    let child = child_process.execFile(cmd, opts, {maxBuffer: 1024 * 1024 * 1000, env: env}, (err, stdout, stderr) => {
      // Log the captured output
      log.write(util.format('*** stdout: %s\n', stdout));
      log.write(util.format('*** stderr: %s\n', stderr));
      log.end();
      
      // Pass the captured output to the callback
      callback(err, stdout, stderr);
    });
    
    child.on('exit', function(code) {
      log.write(util.format('*** %s exited with code %d\n', bin, code));
    });
    
    // Don't setup output logging as it interferes with execFile's output capture
    // this.setupOutputLog(child, log);
    return child;
  }
}
