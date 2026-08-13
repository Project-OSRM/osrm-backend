// Route response validation and geometry processing utilities
import assert from 'node:assert';

import { ensureDecimal } from '../lib/utils.js';
import { env } from './env.js';
import { sendRequest, sendPostRequest } from './http.js';

// Services that support the JSON POST API. Requests to these are additionally issued as a
// POST and checked for equivalence with the GET response (see requestPath).
const POST_SUPPORTED_SERVICES = new Set(['route', 'table', 'match']);

export default class Route {
  constructor(world) {
    this.world = world;
  }

  paramsToQuery(params) {
    let query = '';
    if (params.coordinates !== undefined) {
      // FIXME this disables passing the output if its a default
      // Remove after #2173 is fixed.
      const outputString =
        params.output && params.output !== 'json' ? `.${params.output}` : '';
      query = params.coordinates.join(';') + outputString;
      delete params.coordinates;
      delete params.output;
    }
    if (Object.keys(params).length) {
      query += `?${Object.keys(params)
        .map((k) => `${k}=${params[k]}`)
        .join('&')}`;
    }

    return query;
  }

  // Converts the URL-style `params` object (string encodings) into the JSON body accepted by
  // the POST API. Read-only: does not mutate `params` (paramsToQuery still needs it).
  paramsToJSONBody(params) {
    const toBool = (v) => (v === 'true' ? true : v === 'false' ? false : v);
    const splitList = (v, mapper) => v.split(';').map((e) => (e === '' ? null : mapper(e)));
    // Semicolon-separated integers (indices for sources/destinations/waypoints, seconds for
    // match timestamps).
    const toIntArray = (v) =>
      v.split(';').map((i) => {
        const n = Number(i);
        // Pass invalid entries through unchanged so the backend errors just like GET does.
        return Number.isInteger(n) ? n : i;
      });

    const body = {};
    for (const key of Object.keys(params)) {
      const value = params[key];
      switch (key) {
      case 'coordinates':
        if (Array.isArray(value) && value.length === 1 && /^polyline6?\(/.test(value[0])) {
          body.coordinates = value[0];
        } else {
          body.coordinates = value.map((c) => c.split(',').map(Number));
        }
        break;
      case 'output':
        if (value && value !== 'json') body.format = value;
        break;
      case 'steps':
      case 'generate_hints':
      case 'skip_waypoints':
      case 'tidy':
        body[key] = toBool(value);
        break;
      case 'alternatives':
        body.alternatives = /^\d+$/.test(value) ? Number(value) : toBool(value);
        break;
      case 'continue_straight':
        body.continue_straight = value === 'default' ? 'default' : toBool(value);
        break;
      case 'annotations':
        body.annotations =
          value === 'true' || value === 'false' ? toBool(value) : value.split(',');
        break;
      case 'exclude':
        body.exclude = value.split(',');
        break;
      case 'bearings':
        body.bearings = splitList(value, (b) => b.split(',').map(Number));
        break;
      case 'radiuses':
        body.radiuses = splitList(value, (r) => (r === 'unlimited' ? 'unlimited' : Number(r)));
        break;
      case 'hints':
        body.hints = splitList(value, (h) => h);
        break;
      case 'approaches':
        body.approaches = splitList(value, (a) => a);
        break;
      case 'sources':
      case 'destinations':
      case 'waypoints':
      case 'timestamps':
        body[key] = toIntArray(value);
        break;
      case 'fallback_speed':
      case 'scale_factor':
        body[key] = Number(value);
        break;
      case 'geometries':
      case 'overview':
      case 'snapping':
      case 'fallback_coordinate':
      case 'gaps':
        body[key] = value;
        break;
      default:
        // Unknown / service-specific param: pass through unchanged. The backend ignores
        // unknown keys, and success-only comparison surfaces any real divergence.
        body[key] = value;
      }
    }
    return body;
  }

  getRequest(url) {
    return new Promise((resolve, reject) => {
      sendRequest(url, this.log, (err, res, body) =>
        err ? reject(err) : resolve({ res, body }));
    });
  }

  postRequest(url, jsonBody) {
    return new Promise((resolve, reject) => {
      sendPostRequest(url, jsonBody, this.log, (err, res, body) =>
        err ? reject(err) : resolve({ res, body }));
    });
  }

  // Asserts that a POST response is equivalent to the GET response. Successful responses must
  // be byte/deep-identical; error responses only need to report the same `code` (the GET and
  // POST paths word their diagnostic messages differently).
  assertGetPostEquivalent(getResult, postResult) {
    assert.strictEqual(
      postResult.res.statusCode,
      getResult.res.statusCode,
      `GET/POST status code mismatch: GET=${getResult.res.statusCode} ` +
        `POST=${postResult.res.statusCode}\nGET body: ${getResult.body}\nPOST body: ${postResult.body}`,
    );

    const contentType = getResult.res.headers['content-type'] || '';
    if (!contentType.includes('json')) {
      // flatbuffers or other binary payloads: compare raw bytes.
      assert.strictEqual(postResult.body, getResult.body, 'GET and POST binary responses differ');
      return;
    }

    const getJson = JSON.parse(getResult.body);
    let postJson;
    try {
      postJson = JSON.parse(postResult.body);
    } catch {
      throw new Error(`POST response is not valid JSON: ${postResult.body}`);
    }

    if (getJson.code && getJson.code !== 'Ok') {
      assert.strictEqual(
        postJson.code,
        getJson.code,
        `GET/POST error code mismatch: GET=${getJson.code} POST=${postJson.code}`,
      );
      return;
    }

    assert.deepStrictEqual(
      postJson,
      getJson,
      'GET and POST responses differ for equivalent request',
    );
  }

  requestPath(service, parameters, callback) {
    const supportsPost = POST_SUPPORTED_SERVICES.has(service);

    // Build the JSON body before paramsToQuery mutates `parameters`.
    const jsonBody = supportsPost ? this.paramsToJSONBody(parameters) : null;

    const baseUrl = new URL(`${service}/v1/${this.profile}/`, env.wp.host);
    const query = this.paramsToQuery(parameters);
    const url = new URL(query, baseUrl);

    if (!supportsPost) {
      return sendRequest(url, this.log, callback);
    }

    // Run GET and POST in parallel and verify equivalence, then return the GET result so the
    // existing expectations keep validating the GET response.
    const postUrl = new URL(`${service}/v1/${this.profile}`, env.wp.host);
    Promise.all([this.getRequest(url), this.postRequest(postUrl, jsonBody)])
      .then(([getResult, postResult]) => {
        this.assertGetPostEquivalent(getResult, postResult);
        callback(null, getResult.res, getResult.body);
      })
      .catch((err) => callback(err));
  }

  requestUrl(path, callback) {
    const url = new URL(path, env.wp.host);
    sendRequest(url, this.log, callback);
  }

  // Issues a POST against a verbatim path, for scenarios that exercise the HTTP layer itself
  // rather than a routing result.
  postUrl(path, jsonBody, callback) {
    const url = new URL(path, env.wp.host);
    sendPostRequest(url, jsonBody, this.log, callback);
  }

  // Overwrites the default values in defaults
  // e.g. [[a, 1], [b, 2]], [[a, 5], [d, 10]] => [[a, 5], [b, 2], [d, 10]]
  overwriteParams(defaults, other) {
    const otherMap = {};
    for (const key in other) otherMap[key] = other[key];
    return Object.assign({}, defaults, otherMap);
  }

  encodeWaypoints(waypoints) {
    return waypoints.map((w) => [w.lon, w.lat].map(ensureDecimal).join(','));
  }

  requestRoute(waypoints, bearings, approaches, userParams, callback) {
    if (bearings.length && bearings.length !== waypoints.length)
      throw new Error(
        '*** number of bearings does not equal the number of waypoints',
      );
    if (approaches.length && approaches.length !== waypoints.length)
      throw new Error(
        '*** number of approaches does not equal the number of waypoints',
      );

    const defaults = {
        output: 'json',
        steps: 'true',
        alternatives: 'false',
      },
      params = this.overwriteParams(defaults, userParams),
      encodedWaypoints = this.encodeWaypoints(waypoints);

    params.coordinates = encodedWaypoints;

    if (bearings.length) {
      params.bearings = bearings
        .map((b) => {
          if (b === '*') return '';
          const bs = b.split(',');
          if (bs.length === 2) return b;
          else return (b += ',10');
        })
        .join(';');
    }

    if (approaches.length) {
      params.approaches = approaches.join(';');
    }
    return this.requestPath('route', params, callback);
  }

  requestNearest(node, userParams, callback) {
    const defaults = {
        output: 'json',
      },
      params = this.overwriteParams(defaults, userParams);
    params.coordinates = [[node.lon, node.lat].join(',')];

    return this.requestPath('nearest', params, callback);
  }

  requestTable(waypoints, userParams, callback) {
    const defaults = {
        output: 'json',
      },
      params = this.overwriteParams(defaults, userParams);

    params.coordinates = waypoints.map((w) =>
      [w.coord.lon, w.coord.lat].join(','),
    );
    const srcs = waypoints
        .map((w, i) => [w.type, i])
        .filter((w) => w[0] === 'src')
        .map((w) => w[1]),
      dsts = waypoints
        .map((w, i) => [w.type, i])
        .filter((w) => w[0] === 'dst')
        .map((w) => w[1]);
    if (srcs.length) params.sources = srcs.join(';');
    if (dsts.length) params.destinations = dsts.join(';');

    return this.requestPath('table', params, callback);
  }

  requestTrip(waypoints, userParams, callback) {
    const defaults = {
        output: 'json',
        steps: 'true',
      },
      params = this.overwriteParams(defaults, userParams);

    params.coordinates = this.encodeWaypoints(waypoints);

    return this.requestPath('trip', params, callback);
  }

  requestMatching(waypoints, timestamps, userParams, callback) {
    const defaults = {
        output: 'json',
      },
      params = this.overwriteParams(defaults, userParams);

    params.coordinates = this.encodeWaypoints(waypoints);

    if (timestamps.length) {
      params.timestamps = timestamps.join(';');
    }

    return this.requestPath('match', params, callback);
  }

  extractInstructionList(instructions, keyFinder) {
    if (instructions) {
      return instructions.legs
        .reduce((m, v) => m.concat(v.steps), [])
        .map(keyFinder)
        .join(',');
    }
  }

  summary(instructions) {
    if (instructions) {
      return instructions.legs.map((l) => l.summary).join(';');
    }
  }

  wayList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.name);
  }

  refList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.ref || '');
  }

  pronunciationList(instructions) {
    return this.extractInstructionList(
      instructions,
      (s) => s.pronunciation || '',
    );
  }

  destinationsList(instructions) {
    return this.extractInstructionList(
      instructions,
      (s) => s.destinations || '',
    );
  }

  exitsList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.exits || '');
  }

  reverseBearing(bearing) {
    if (bearing >= 180) return bearing - 180;
    return bearing + 180;
  }

  bearingList(instructions) {
    return this.extractInstructionList(
      instructions,
      (s) =>
        `${
          'in' in s.intersections[0]
            ? this.reverseBearing(
              s.intersections[0].bearings[s.intersections[0].in],
            )
            : 0
        }->${
          'out' in s.intersections[0]
            ? s.intersections[0].bearings[s.intersections[0].out]
            : 0
        }`,
    );
  }

  lanesList(instructions) {
    return this.extractInstructionList(instructions, (s) => {
      return s.intersections
        .map((i) => {
          if (i.lanes) {
            return i.lanes
              .map((l) => {
                const indications = l.indications.join(';');
                return `${indications}:${l.valid ? 'true' : 'false'}`;
              })
              .join(' ');
          } else {
            return '';
          }
        })
        .join(';');
    });
  }

  approachList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.approaches || '');
  }

  annotationList(instructions) {
    if (!('annotation' in instructions.legs[0])) return '';

    const merged = {};
    instructions.legs.map((l) => {
      Object.keys(l.annotation)
        .filter((a) => !a.match(/metadata/))
        .forEach((a) => {
          if (!merged[a]) merged[a] = [];
          merged[a].push(l.annotation[a].join(':'));
        });
      if (l.annotation.metadata) {
        merged.metadata = {};
        Object.keys(l.annotation.metadata).forEach((a) => {
          if (!merged.metadata[a]) merged.metadata[a] = [];
          merged.metadata[a].push(l.annotation.metadata[a].join(':'));
        });
      }
    });
    Object.keys(merged)
      .filter((k) => !k.match(/metadata/))
      .map((a) => {
        merged[a] = merged[a].join(',');
      });
    if (merged.metadata) {
      Object.keys(merged.metadata).map((a) => {
        merged.metadata[a] = merged.metadata[a].join(',');
      });
    }
    return merged;
  }

  alternativesList(instructions) {
    // alternatives_count come from tracepoints list
    return instructions.tracepoints
      .map((t) => t.alternatives_count.toString())
      .join(',');
  }

  turnList(instructions) {
    return instructions.legs
      .reduce((m, v) => m.concat(v.steps), [])
      .map((v) => {
        switch (v.maneuver.type) {
        case 'depart':
        case 'arrive':
          return v.maneuver.type;
        case 'on ramp':
        case 'off ramp':
          return `${v.maneuver.type} ${v.maneuver.modifier}`;
        case 'roundabout':
          return `roundabout-exit-${v.maneuver.exit}`;
        case 'rotary':
          if ('rotary_name' in v)
            return `${v.rotary_name}-exit-${v.maneuver.exit}`;
          else return `rotary-exit-${v.maneuver.exit}`;
        case 'roundabout turn':
          return `${v.maneuver.type} ${v.maneuver.modifier} exit-${v.maneuver.exit}`;
          // FIXME this is a little bit over-simplistic for merge/fork instructions
        default:
          return `${v.maneuver.type} ${v.maneuver.modifier}`;
        }
      })
      .join(',');
  }

  locations(instructions) {
    return instructions.legs
      .reduce((m, v) => m.concat(v.steps), [])
      .map((v) => {
        return this.findNodeByLocation(v.maneuver.location);
      })
      .join(',');
  }

  intersectionList(instructions) {
    return instructions.legs
      .reduce((m, v) => m.concat(v.steps), [])
      .map((v) => {
        return v.intersections
          .map((intersection) => {
            let string = `${intersection.entry[0]}:${intersection.bearings[0]}`,
              i;
            for (i = 1; i < intersection.bearings.length; ++i)
              string = `${string} ${intersection.entry[i]}:${intersection.bearings[i]}`;
            return string;
          })
          .join(',');
      })
      .join(';');
  }

  modeList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.mode);
  }

  drivingSideList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.driving_side);
  }

  classesList(instructions) {
    return this.extractInstructionList(
      instructions,
      (s) =>
        `[${s.intersections.map((i) => `(${i.classes ? i.classes.join(',') : ''})`).join(',')}]`,
    );
  }

  timeList(instructions) {
    return this.extractInstructionList(instructions, (s) => `${s.duration}s`);
  }

  distanceList(instructions) {
    return this.extractInstructionList(instructions, (s) => `${s.distance}m`);
  }

  weightName(instructions) {
    return instructions ? instructions.weight_name : '';
  }

  weightList(instructions) {
    return this.extractInstructionList(instructions, (s) => s.weight);
  }
}
