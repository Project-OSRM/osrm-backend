var linereader = require('readline').createInterface( {
    input: require('fs').createReadStream(require('path').join(__dirname, '..', 'CHANGELOG.md'))
});

var done = false;
var linenum = 0;
var has_errors = false;
linereader.on('line', function(line) {
    linenum += 1;
    // Only validate the `# UNRELEASED` section
    if (line.match(/^# [^U]/)) done = true;
    if (done) return;

    var line_errors = [];

    if (line.match(/^ {6}/)) {
        if (!line.match(/^ {6}- (ADDED|FIXED|CHANGED|REMOVED): /)) {
            line_errors.push("ERROR: changelog entries must start with '- (ADDED|FIXED|CHANGED|REMOVED): '");
        }
        if (!line.match(/\[#[0-9]+\]\(http.*\)$/)) {
            line_errors.push("ERROR: changelog entries must end with an issue or PR link in Markdown format");
        }
    }

    if (line_errors.length > 0) {
      has_errors = true;

      // Coloured output if it's directly on an interactive terminal
      if (process.stdout.isTTY) {
          console.log('\x1b[31mERROR ON LINE %d\x1b[0m: %s', linenum, line);
          for (var i = 0; i<line_errors.length; i++) {
              console.log('    \x1b[33m%s\x1b[0m', line_errors[i]);
          }
      } else {
          console.log('ERROR ON LINE %d: %s', linenum, line);
          for (var i = 0; i<line_errors.length; i++) {
              console.log('    %s', line_errors[i]);
          }
      }
    }

});

linereader.on('close', function() {
    process.exit(has_errors ? 1 : 0);
});
