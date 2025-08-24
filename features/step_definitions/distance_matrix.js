// Step definitions for testing distance/duration matrix API endpoints
import util from 'util';

import flatbuffers from 'flatbuffers';
import { osrm } from '../support/fbresult_generated.js';
const FBResult = osrm.engine.api.fbresult.FBResult;
import { When } from '@cucumber/cucumber';

// Regular expressions for matching different matrix API test scenarios
const durationsRegex = new RegExp(
  /^I request a travel time matrix I should get$/
);
const durationsCodeOnlyRegex = new RegExp(
  /^I request a travel time matrix with these waypoints I should get the response code$/
);
const distancesRegex = new RegExp(
  /^I request a travel distance matrix I should get$/
);
const estimatesRegex = new RegExp(
  /^I request a travel time matrix I should get estimates for$/
);
const durationsRegexFb = new RegExp(
  /^I request a travel time matrix with flatbuffers I should get$/
);
const distancesRegexFb = new RegExp(
  /^I request a travel distance matrix with flatbuffers I should get$/
);

// Sentinel values for unreachable routes in matrix responses
const DURATIONS_NO_ROUTE = 2147483647; // MAX_INT (seconds)
const DISTANCES_NO_ROUTE = 3.40282e38; // MAX_FLOAT (meters)

const FORMAT_JSON = 'json';
const FORMAT_FB = 'flatbuffers';

When(durationsRegex, function (table, callback) {
  tableParse.call(
    this,
    table,
    DURATIONS_NO_ROUTE,
    'durations',
    FORMAT_JSON,
    callback
  );
});
When(durationsCodeOnlyRegex, function (table, callback) {
  tableCodeOnlyParse.call(this, table, 'durations', FORMAT_JSON, callback);
});
When(distancesRegex, function (table, callback) {
  tableParse.call(
    this,
    table,
    DISTANCES_NO_ROUTE,
    'distances',
    FORMAT_JSON,
    callback
  );
});
When(estimatesRegex, function (table, callback) {
  tableParse.call(
    this,
    table,
    DISTANCES_NO_ROUTE,
    'fallback_speed_cells',
    FORMAT_JSON,
    callback
  );
});
When(durationsRegexFb, function (table, callback) {
  tableParse.call(
    this,
    table,
    DURATIONS_NO_ROUTE,
    'durations',
    FORMAT_FB,
    callback
  );
});
When(distancesRegexFb, function (table, callback) {
  tableParse.call(
    this,
    table,
    DISTANCES_NO_ROUTE,
    'distances',
    FORMAT_FB,
    callback
  );
});

const durationsParse = function (v) {
  return isNaN(parseInt(v));
};
const distancesParse = function (v) {
  return isNaN(parseFloat(v));
};
const estimatesParse = function (v) {
  return isNaN(parseFloat(v));
};

function tableCodeOnlyParse(table, annotation, format, callback) {
  const params = this.queryParams;
  params.annotations =
    ['durations', 'fallback_speed_cells'].indexOf(annotation) !== -1
      ? 'duration'
      : 'distance';
  params.output = format;

  let got;

  this.reprocessAndLoadData((e) => {
    if (e) return callback(e);
    const testRow = function (row, ri, cb) {
      const afterRequest = function (err, res, body) {
        if (err) return cb(err);

        for (const k in row) {
          const match = k.match(/param:(.*)/);
          if (match) {
            if (row[k] === '(nil)') {
              params[match[1]] = null;
            } else if (row[k]) {
              params[match[1]] = [row[k]];
            }
            got[k] = row[k];
          }
        }

        let json;
        got.code = 'unknown';
        if (body.length) {
          json = JSON.parse(body);
          got.code = json.code;
        }

        cb(null, got);
      }.bind(this);

      const params = this.queryParams,
        waypoints = [];
      if (row.waypoints) {
        row.waypoints.split(',').forEach((n) => {
          const node = this.findNodeByName(n);
          if (!node)
            throw new Error(
              util.format('*** unknown waypoint node "%s"', n.trim())
            );
          waypoints.push({ coord: node, type: 'loc' });
        });
        got = { waypoints: row.waypoints };

        this.requestTable(waypoints, params, afterRequest);
      } else {
        throw new Error('*** no waypoints');
      }
    }.bind(this);

    this.processRowsAndDiff(table, testRow, callback);
  });
}

function tableParse(table, noRoute, annotation, format, callback) {
  const parse =
    annotation == 'distances'
      ? distancesParse
      : annotation == 'durations'
        ? durationsParse
        : estimatesParse;
  const params = this.queryParams;
  params.annotations =
    ['durations', 'fallback_speed_cells'].indexOf(annotation) !== -1
      ? 'duration'
      : 'distance';
  params.output = format;

  const tableRows = table.raw();

  if (tableRows[0][0] !== '')
    throw new Error('*** Top-left cell of matrix table must be empty');

  const waypoints = [],
    columnHeaders = tableRows[0].slice(1),
    rowHeaders = tableRows.map((h) => h[0]).slice(1),
    symmetric =
      columnHeaders.length == rowHeaders.length &&
      columnHeaders.every((ele, i) => ele === rowHeaders[i]);

  if (symmetric) {
    columnHeaders.forEach((nodeName) => {
      const node = this.findNodeByName(nodeName);
      if (!node)
        throw new Error(util.format('*** unknown node "%s"', nodeName));
      waypoints.push({ coord: node, type: 'loc' });
    });
  } else {
    columnHeaders.forEach((nodeName) => {
      const node = this.findNodeByName(nodeName);
      if (!node)
        throw new Error(util.format('*** unknown node "%s"', nodeName));
      waypoints.push({ coord: node, type: 'dst' });
    });
    rowHeaders.forEach((nodeName) => {
      const node = this.findNodeByName(nodeName);
      if (!node)
        throw new Error(util.format('*** unknown node "%s"', nodeName));
      waypoints.push({ coord: node, type: 'src' });
    });
  }

  this.reprocessAndLoadData((e) => {
    if (e) return callback(e);
    // compute matrix

    this.requestTable(waypoints, params, (err, response, body) => {
      if (err) return callback(err);
      if (!body.length) return callback(new Error('Invalid response body'));

      let result = [];
      if (format === 'json') {
        const json = JSON.parse(body);

        if (annotation === 'fallback_speed_cells') {
          result = table.raw().map((row) => row.map(() => ''));
          json[annotation].forEach((pair) => {
            result[pair[0] + 1][pair[1] + 1] = 'Y';
          });
          result = result.slice(1).map((row) => {
            const hashes = {};
            row.slice(1).forEach((v, i) => {
              hashes[tableRows[0][i + 1]] = v;
            });
            return hashes;
          });
        } else {
          result = json[annotation].map((row) => {
            const hashes = {};
            row.forEach((v, i) => {
              hashes[tableRows[0][i + 1]] = parse(v) ? '' : v;
            });
            return hashes;
          });
        }
      } else {
        //flatbuffers
        const bytes = new Uint8Array(body.length);
        for (let indx = 0; indx < body.length; ++indx) {
          bytes[indx] = body.charCodeAt(indx);
        }
        const buf = new flatbuffers.ByteBuffer(bytes);
        const fb = FBResult.getRootAsFBResult(buf);

        let matrix;
        if (annotation === 'durations') {
          matrix = fb.table().durationsArray();
        }
        if (annotation === 'distances') {
          matrix = fb.table().distancesArray();
        }
        const cols = fb.table().cols();
        const rows = fb.table().rows();
        for (let r = 0; r < rows; ++r) {
          result[r] = {};
          for (let c = 0; c < cols; ++c) {
            result[r][tableRows[0][c + 1]] = matrix[r * cols + c];
          }
        }
      }

      const testRow = function (row, ri, cb) {
        for (const k in result[ri]) {
          if (this.FuzzyMatch.match(result[ri][k], row[k])) {
            result[ri][k] = row[k];
          } else if (row[k] === '' && result[ri][k] === noRoute) {
            result[ri][k] = '';
          } else {
            result[ri][k] = result[ri][k].toString();
          }
        }

        result[ri][''] = row[''];
        cb(null, result[ri]);
      }.bind(this);

      this.processRowsAndDiff(table, testRow, callback);
    });
  });
}
