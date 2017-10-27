// ExpressJS middleware for OSRM

const parameterParsers = {
    bearings: (req, result) => {
        if (/^(\d+)(,\d+)?(;(\d+)(,\d+)?)*$/.test(req.query.bearings)) {
            result.bearings = req.query.bearings
                .split(';')
                .map(b => b.split(',').map(n => parseInt(n)));
        } else {
            throw Error('bearings must be a semicolon-separated list of positive integers or comma-separated positive integer pairs, like \'90,4;90;10\'');
        }
    },
    radiuses: (req, result) => {
        if (/^(\d+(\.\d+)?|unlimited)(;(\d+(\.\d+)?|unilmited))*$/.test(req.query.radiuses)) {
            result.radiuses = req.query.radiuses
                .split(';')
                .map(n => parseFloat(n));
        } else {
            throw Error('radiuses must be a semicolon-separated list of positive decimals, or the word \'unlimited\', like \'unlimited;30.5;15;unlimited\'');
        }
    },
    generate_hints: (req, result) => {
        if (/^true|false$/.test(req.query.generate_hints)) {
            result.generate_hints = req.query.generate_hints === 'true';
        } else {
            throw Error('generate_hints must be \'true\' or \'false\'');
        }
    },
    hints: () => {
        // Do nothing with hints, only for backward compat
    },
    approaches: (req, result) => {
        if (/^(curb|unrestricted)(;(curb|unrestricted))*$/.test(req.query.approaches)) {
            result.approaches = req.query.approaches.split(';');
        } else {
            throw Error('approaches must be a semicolon-separated list of \'curb\' or \'unrestricted\', like \'curb;curb;unrestricted;unrestricted\'');
        }
    },
    exclude: (req, result) => {
        if (/^([^,]+)(,[^,]+)*$/.test(req.query.exclude)) {
            result.exclude = req.query.exclude.split(',');
        } else {
            throw Error('exclude must be a comma-separated list of words');
        }
    },
    alternatives: (req, result) => {
        if (/^true|false|\d+$/.test(req.query.alternatives)) {
            if (req.query.alternatives === 'true') {
                result.alternatives = true;
            } else if (req.query.alternatives === 'false') {
                result.alternatives = false;
            } else {
                result.alternatives = parseInt(req.query.alternatives);
            }
        } else {
            throw Error('alternatives must be either \'true\', \'false\', or a positive integer');
        }
    },
    steps: (req, result) => {
        if (/^true|false$/.test(req.query.steps)) {
            result.steps = req.query.steps === 'true';
        } else {
            throw Error('steps must be either \'true\' or \'false\'');
        }
    },
    annotations: (req, result) => {
        if (
            /^true|false|((nodes|distance|duration|datasources|weight|speed)(,(nodes|distance|duration|datasources|weight|speed))*)$/.test(req.query.annotations)
        ) {
            result.annotations =
                req.query.annotations === 'true' ||
                req.query.annotations.split(',');
        } else {
            throw Error('annotations must be \'true\', \'false\', or a comma-separated list of \'nodes\',\'distance\',\'duration\',\'datasources\',\'weight\',\'speed\'');
        }
    },
    geometries: (req, result) => {
        if (/^polyline|polyline6|geojson$/.test(req.query.geometries)) {
            result.geometries = req.query.geometries;
        } else {
            throw Error('geometries must be \'polyline\', \'polyline6\', or \'geojson\'');
        }
    },
    overview: (req, result) => {
        if (/^simplified|full|false$/.test(req.query.overview)) {
            result.overview = req.query.overview;
        } else {
            throw Error('overview must be \'simplified\', \'full\', or \'false\'');
        }
    },
    continue_straight: (req, result) => {
        if (/^default|true|false$/.test(req.query.continue_straight)) {
            result.continue_straight =
                req.query.continue_straight === 'default'
                    ? null
                    : req.query.continue_straight === 'true';
        } else {
            throw Error('continue_straight must be \'default\', \'true\' or \'false\'');
        }
    },
    sources: (req, result) => {
        if (/^\d+(;\d+)*$/.test(req.query.sources)) {
            result.sources = req.query.sources.split(';').map(s => parseInt(s));
        } else {
            throw Error('sources must be a semicolon-separated list of positive integers, like \'1;2;3\'');
        }
    },
    destinations: (req, result) => {
        if (/^\d+(;\d+)*$/.test(req.query.destinations)) {
            result.destinations = req.query.destinations
                .split(';')
                .map(s => parseInt(s));
        } else {
            throw Error('destinations must be a semicolon-separated list of positive integers, like \'1;2;3\'');
        }
    },
    timestamps: (req, result) => {
        if (/^\d+(;\d+)*$/.test(req.query.timestamps)) {
            result.timestamps = req.query.timestamps
                .split(';')
                .map(s => parseInt(s));
        } else {
            throw Error('timestamps must be a semicolon-separated list of positive integers, like \'1;2;3\'');
        }
    },
    gaps: (req, result) => {
        if (/^ignore|split$/.test(req.query.gaps)) {
            result.gaps = req.query.gaps;
        } else {
            throw Error('gaps must be either \'ignore\' or \'split\'');
        }
    },
    tidy: (req, result) => {
        if (/^true|false$/.test(req.query.tidy)) {
            result.tidy = req.query.tidy === 'true';
        } else {
            throw Error('tidy must be \'true\' or \'false\'');
        }
    },
    source: (req, result) => {
        if (/^any|first$/.test(req.query.source)) {
            result.source = req.query.source;
        } else {
            throw Error('source must be \'any\' or \'first\'');
        }
    },
    destination: (req, result) => {
        if (/^any|first|last$/.test(req.query.destination)) {
            result.destination = req.query.destination;
        } else {
            throw Error('destination must be \'any\', \'first\', or \'last\'');
        }
    },
    roundtrip: (req, result) => {
        if (/^true|false$/.test(req.query.roundtrip)) {
            result.roundtrip = req.query.roundtrip === 'true';
        } else {
            throw Error('roundtrip must be \'true\', \'false\'');
        }
    },
    number: (req, result) => {
        if (/^\d+$/.test(req.query.number)) {
            result.number = parseInt(req.query.number);
        } else {
            throw Error('number must be a positive integer');
        }
    }
};

function parseParameter(fieldname,req,result) {
    if (fieldname in req.query) parameterParsers[fieldname](req,result);
}

function parseCoordinates(req, result) {
    var coordinates = (req.params.coordinates || '').split(';');
    result.coordinates = coordinates.map(c =>
        c.split(',').map(n => parseFloat(n))
    );
    // TODO: validate float parsing here maybe?
}

function baseOptions(req, results) {
    parseParameter('bearings', req, results);
    parseParameter('radiuses', req, results);
    parseParameter('generate_hints', req, results);
    parseParameter('hints', req, results);
    parseParameter('approaches', req, results);
    parseParameter('exclude', req, results);
}

function routeOptions(req, results) {
    parseParameter('alternatives', req, results);
    parseParameter('steps', req, results);
    parseParameter('annotations', req, results);
    parseParameter('overview', req, results);
    parseParameter('geometries', req, results);
    parseParameter('continue_straight', req, results);
}

function tableOptions(req, results) {
    parseParameter('sources', req, results);
    parseParameter('destinations', req, results);
}

function matchOptions(req, results) {
    parseParameter('timestamps', req, results);
    parseParameter('gaps', req, results);
    parseParameter('tidy', req, results);
    parseParameter('annotations', req, results);
    parseParameter('overview', req, results);
    parseParameter('geometries', req, results);
}

function tripOptions(req, results) {
    parseParameter('source', req, results);
    parseParameter('destination', req, results);
    parseParameter('roundtrip', req, results);
    parseParameter('steps', req, results);
    parseParameter('annotations', req, results);
    parseParameter('overview', req, results);
    parseParameter('geometries', req, results);
}

function nearestOptions(req, results) {
    parseParameter('number', req, results);
}

function checkUnused(req, query) {
    Object.keys(req.query).forEach((key) => {
        if (!(key in query)) {
            throw Error(`Unrecognized parameter '${key}'`);
        }
    });
}

var OSRM = module.exports = require('./binding/node_osrm.node').OSRM;

OSRM.nearestOptions = nearestOptions;
OSRM.routeOptions = routeOptions;
OSRM.baseOptions = baseOptions;
OSRM.tripOptions = tripOptions;
OSRM.tableOptions = tableOptions;
OSRM.matchOptions = matchOptions;
OSRM.parseCoordinates = parseCoordinates;

OSRM.middleware = function(osrm) {
    return {
        route: (req, res /*, next */) => {
            try {
                var query = {};
                parseCoordinates(req, query);
                baseOptions(req, query);
                routeOptions(req, query);
                checkUnused(req, query);
                osrm.route(query, function(err, result) {
                    if (err)
                        return res.status(400).json({
                            code: err.message.split(': ')[0],
                            message: err.message.split(': ')[1]
                        });
                    result.code = 'Ok';
                    return res.json(result);
                });
            } catch (err) {
                return res
                    .status(400)
                    .json({ code: 'Error', message: err.message });
            }
        },
        table: (req, res /* , next */) => {
            try {
                var query = {};
                parseCoordinates(req, query);
                baseOptions(req, query);
                tableOptions(req, query);
                checkUnused(req, query);
                osrm.table(query, function(err, result) {
                    if (err) {
                        return res.status(400).json({
                            code: err.message.split(': ')[0],
                            message: err.message.split(': ')[1]
                        });
                    }
                    result.code = 'Ok';
                    return res.json(result);
                });
            } catch (err) {
                return res
                    .status(400)
                    .json({ code: 'Error', message: err.message });
            }
        },
        match: (req, res /*, next */) => {
            try {
                var query = {};
                parseCoordinates(req, query);
                baseOptions(req, query);
                matchOptions(req, query);
                checkUnused(req, query);
                osrm.match(query, function(err, result) {
                    if (err) {
                        return res.status(400).json({
                            code: err.message.split(': ')[0],
                            message: err.message.split(': ')[1]
                        });
                    }
                    result.code = 'Ok';
                    return res.json(result);
                });
            } catch (err) {
                return res
                    .status(400)
                    .json({ code: 'Error', message: err.message });
            }
        },
        trip: (req, res /*, next */) => {
            try {
                var query = {};
                parseCoordinates(req, query);
                baseOptions(req, query);
                tripOptions(req, query);
                checkUnused(req, query);
                osrm.trip(query, function(err, result) {
                    if (err) {
                        return res.status(400).json({
                            code: err.message.split(': ')[0],
                            message: err.message.split(': ')[1]
                        });
                    }
                    result.code = 'Ok';
                    return res.json(result);
                });
            } catch (err) {
                return res
                    .status(400)
                    .json({ code: 'Error', message: err.message });
            }
        },
        tile: (req, res /*, next */) => {
            try {
                var query = {};
                baseOptions(req, query);
                checkUnused(req, query);
                osrm.tile(query, function(err, result) {
                    if (err) {
                        return res.status(400).json({
                            code: err.message.split(': ')[0],
                            message: err.message.split(': ')[1]
                        });
                    }
                    result.code = 'Ok';
                    return res.json(result);
                });
            } catch (err) {
                return res
                    .status(400)
                    .json({ code: 'Error', message: err.message });
            }
        },
        nearest: (req, res /*, next */) => {
            try {
                var query = {};
                parseCoordinates(req, query);
                nearestOptions(req, query);
                checkUnused(req, query);
                osrm.nearest(query, function(err, result) {
                    if (err) {
                        return res.status(400).json({
                            code: err.message.split(': ')[0],
                            message: err.message.split(': ')[1]
                        });
                    }
                    result.code = 'Ok';
                    return res.json(result);
                });
            } catch (err) {
                return res
                    .status(400)
                    .json({ code: 'Error', message: err.message });
            }
        }
    };
};

OSRM.version = require('../package.json').version;
