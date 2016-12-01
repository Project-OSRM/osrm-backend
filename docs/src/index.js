'use strict';

/**
 * Brand names, in order to decreasing length, for different
 * media queries.
 */
module.exports.brandNames = {
  desktop: 'OSRM API Documentation',
  tablet: 'OSRM API Docs',
  mobile: 'OSRM API'
};

/**
 * Classes that define the top-left brand box.
 */
module.exports.brandClasses = 'fill-red';


/**
 * Text for the link back to the linking website.
 */
module.exports.backLink = 'Back to project-osrm.org';

/**
 * Runs after highlighting code samples. You can use this
 * hook to, for instance, highlight a token and link it
 * to some canonical part of documentation.
 */
module.exports.postHighlight = function(html) {
  return html;
};

/**
 * Highlight tokens in endpoint URLs, optionally linking to documentation
 * or adding detail. This is the equivalent of postHighlight but it
 * operates on endpoint URLs only.
 */
function highlightTokens(str) {
  return str.replace(/{[\w_]+}/g,
    (str) => '<span class="strong">' + str + '</span>');
}

/**
 * Transform endpoints given as strings in a highlighted block like
 *
 *     ```endpoint
 *     GET /foo/bar
 *     ```
 *
 * Into HTML nodes that format those endpoints in nice ways.
 */
module.exports.transformURL = function(value) {
  let parts = value.split(/\s+/);
  return {
    type: 'html',
    value: `<div class='endpoint dark fill-dark round '>
      <div class='round-left pad0y pad1x fill-lighten0 code small endpoint-method'>${parts[0]}</div>
      <div class='pad0 code small endpoint-url'>${highlightTokens(parts[1])}</div>
    </div>`
  };
};

module.exports.remarkPlugins = [];
