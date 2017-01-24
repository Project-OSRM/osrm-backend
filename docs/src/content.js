var fs = require('fs');

/**
 * This file exports the content of your website, as a bunch of concatenated
 * Markdown files. By doing this explicitly, you can control the order
 * of content without any level of abstraction.
 *
 * Using the brfs module, fs.readFileSync calls in this file are translated
 * into strings of those files' content before the file is delivered to a
 * browser: the content is read ahead-of-time and included in bundle.js.
 */
module.exports =
      '# HTTP API\n' +
      fs.readFileSync('./content/http.md', 'utf8') + '\n'+
      '# libosrm C++ API\n' +
      fs.readFileSync('./content/libosrm.md', 'utf8') + '\n';
