// ExpressJS middleware

function baseoptions(req, query) {
    
        var coordinates = (req.params.coordinates || '').split(';');
        query.coordinates = coordinates.map(c => c.split(",").map(n=>parseFloat(n)));
    
        if ('bearings' in req.query) {
          var m = req.query.bearings.match(/^(\d+)(,\d+)?(;(\d+)(,\d+)?)*$/);
          if (m) { query.bearings = req.query.bearings.split(';').map(b => b.split(',').map(n => parseInt(n))); }
        }
    
        if ('radiuses' in req.query) {
          var m = req.query.radiuses.match(/^(\d+(\.\d+)?|unlimited)(;(\d+(\.\d+)?|unilmited))*$/);
          if (m) { query.radiuses = req.query.radiuses.split(';').map(n => parseFloat(n)); }
        }
    
        if ('generate_hints' in req.query) {
          var m = req.query.generate_hints.match(/^true|false$/);
          if (m) { query.generate_hints = req.query.generate_hints === 'true'; }
        }
    
        if ('hints' in req.query) {
            // TODO: parse hints
        }
    
        if ('approaches' in req.query) {
          var m = req.query.approaches.match(/^(curb|unrestricted)(;(curb|unrestricted))*$/);
          if (m) { query.approaches = req.query.approaches.split(';'); }
        }

        if ('exclude' in req.query) {
          var m = req.query.exclude.match(/^([^,]+)(,[^,]+)*$/)
          if (m) { query.exclude = req.query.exclude.split(','); }
        }
    
}

function routeoptions(req, query) {
    
        if ('alternatives' in req.query) {
          var m = req.query.alternatives.match(/^true|false|\d+$/);
          if (m) {
              switch(req.query.alternatives) {
                  case 'true':
                      query.alternatives = true;
                      break;
                  case 'false':
                      query.alternatives = false;
                      break;
                  default:
                      query.alternatives = parseInt(req.query.alternatives);
                      break;
              }
           }
        } else {
          //query.alternatives = false;
        }
    
        if ('steps' in req.query) {
          var m = req.query.steps.match(/^true|false$/);
          if (m) { query.steps = req.query.steps === 'true'; }
        } else {
          //query.steps = false;
        }
    
        if ('annotations' in req.query) {
          var m = req.query.annotations.match(/^true|false|(nodes|distance|duration|datasources|weight|speed)(,(nodes|distance|duration|datasources|weight|speed))*$/);
          if (m) { query.annotations = (req.query.annotations === 'true') || req.query.annotations.split(','); }
        } else {
          //query.annotations = false;
        }
    
        if ('geometries' in req.query) {
          var m = req.query.geometries.match(/^polyline|polyline6|geojson$/);
          if (m) { query.geometries = req.query.geometries; }
        } else {
          //query.geometries = 'polyline';
        }
    
        if ('overview' in req.query) {
          var m = req.query.overview.match(/^simplified|full|false$/);
          if (m) { query.overview = req.query.overview; }
        } else {
          //query.overview = 'simplified';
        }
    
        if ('continue_straight' in req.query) {
          var m = req.query.continue_straight.match(/^default|true|false$/);
          if (m) { query.continue_straight = req.query.continue_straight === 'default' ? null : (req.query.continue_straight === 'true'); }
        } else {
          //query.continue_straight = null;
        }
}

function tableoptions(req, query) {
  if ('sources' in req.query) {
    var m = req.query.sources.match(/^\d+(;\d+)*$/);
    if (m) {
      query.sources = req.query.sources.split(';').map(s => parseInt(s));
    }
  }
  if ('destinations' in req.query) {
    var m = req.query.destinations.match(/^\d+(;\d+)*$/);
    if (m) {
      query.destinations = req.query.destinations.split(';').map(s => parseInt(s));
    }
  }
}

function matchoptions(req, query) {
  if ('timestamps' in req.query) {
    var m = req.query.timestamps.match(/^\d+(;\d+)*$/);
    if (m) {
      query.timestamps = req.query.timestamps.split(';').map(s => parseInt(s));
    }
  }

  if ('gaps' in req.query) {
    var m = req.query.gaps.match(/^ignore|split$/);
    if (m) {
      query.gaps = req.query.gaps;
    }
  }

  if ('tidy' in req.query) {
    var m = req.query.tidy.match(/^true|false$/);
    if (m) {
      query.tidy = req.query.tidy === 'true';
    }
  }

        if ('annotations' in req.query) {
          var m = req.query.annotations.match(/^true|false|(nodes|distance|duration|datasources|weight|speed)(,(nodes|distance|duration|datasources|weight|speed))*$/);
          if (m) { query.annotations = (req.query.annotations === 'true') || req.query.annotations.split(','); }
        } else {
          //query.annotations = false;
        }

        if ('overview' in req.query) {
          var m = req.query.overview.match(/^simplified|full|false$/);
          if (m) { query.overview = req.query.overview; }
        } else {
          //query.overview = 'simplified';
        }

        if ('geometries' in req.query) {
          var m = req.query.geometries.match(/^polyline|polyline6|geojson$/);
          if (m) { query.geometries = req.query.geometries; }
        }

}

function tripoptions(req, query) {
    if ('source' in req.query) {
        var m = req.query.source.match(/^any|first$/);
        if (m) { query.source = req.query.source; }
    }
    if ('destination' in req.query) {
        var m = req.query.destination.match(/^any|first|last$/);
        if (m) { query.destination = req.query.destination; }
    }
    if ('roundtrip' in req.query) {
        var m = req.query.roundtrip.match(/^true|false$/);
        if (m) { query.roundtrip = req.query.roundtrip === 'true'; }
    }
        if ('steps' in req.query) {
          var m = req.query.steps.match(/^true|false$/);
          if (m) { query.steps = req.query.steps === 'true'; }
        }

        if ('annotations' in req.query) {
          var m = req.query.annotations.match(/^true|false|(nodes|distance|duration|datasources|weight|speed)(,(nodes|distance|duration|datasources|weight|speed))*$/);
          if (m) { query.annotations = (req.query.annotations === 'true') || req.query.annotations.split(','); }
        } else {
          //query.annotations = false;
        }

        if ('overview' in req.query) {
          var m = req.query.overview.match(/^simplified|full|false$/);
          if (m) { query.overview = req.query.overview; }
        } else {
          //query.overview = 'simplified';
        }

        if ('geometries' in req.query) {
          var m = req.query.geometries.match(/^polyline|polyline6|geojson$/);
          if (m) { query.geometries = req.query.geometries; }
        }
}



var OSRM = module.exports = require('./binding/node_osrm.node').OSRM;

OSRM.middleware = function(osrm) {
        return {
          route: (req, res, next) => {
            var query = {};
            baseoptions(req, query);
            routeoptions(req, query);
            try {
                osrm.route(query, function(err, result) { 
                    if (err) return res.status(400).json({'code':err.message.split(": ")[0], 'message':err.message.split(": ")[1]});
                    result.code = "Ok";
                    return res.json(result);
                });
            } catch (err) {
                return res.status(400).json({'code':'Error', 'message':err.message});
            }
          },
          table: (req, res, next) => {
            var query = {};
            baseoptions(req, query);
            tableoptions(req,query);
            try {
            osrm.table(query, function(err, result) { 
                if (err) return res.status(400).json({'code':err.message.split(": ")[0], 'message':err.message.split(": ")[1]});
                result.code = "Ok";
                return res.json(result);
            });
            } catch (err) {
                return res.status(400).json({'code':'Error', 'message':err.message});
            }
          },
          match: (req, res, next) => {
            var query = {};
            baseoptions(req, query);
            matchoptions(req, query);
            try {
            osrm.match(query, function(err, result) { 
                if (err) return res.status(400).json({'code':err.message.split(": ")[0], 'message':err.message.split(": ")[1]});
                result.code = "Ok";
                return res.json(result);
            });
            } catch (err) {
                return res.status(400).json({'code':'Error', 'message':err.message});
            }
          },
          trip: (req, res, next) => {
            var query = {};
            baseoptions(req, query);
            tripoptions(req, query);
            try {
            osrm.trip(query, function(err, result) { 
                if (err) return res.status(400).json({'code':err.message.split(": ")[0], 'message':err.message.split(": ")[1]});
                result.code = "Ok";
                return res.json(result);
            });
            } catch (err) {
                return res.status(400).json({'code':'Error', 'message':err.message});
            }
          },
          tile: (req, res, next) => {
            var query = {};
            baseoptions(req, query);
            try {
            osrm.tile(query, function(err, result) { 
                if (err) return res.status(400).json({'code':err.message.split(": ")[0], 'message':err.message.split(": ")[1]});
                result.code = "Ok";
                return res.json(result);
            });
            } catch (err) {
                return res.status(400).json({'code':'Error', 'message':err.message});
            }
          },
          nearest: (req, res, next) => {
            var query = {};
            baseoptions(req, query);
            try {
            osrm.nearest(query, function(err, result) {
                if (err) return res.status(400).json({'code':err.message.split(": ")[0], 'message':err.message.split(": ")[1]});
                result.code = "Ok";
                return res.json(result);
            });
            } catch (err) {
                return res.status(400).json({'code':'Error', 'message':err.message});
            }
          }
        };
    };

OSRM.version = require('../package.json').version;
