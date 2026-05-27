#!/usr/bin/env node
// Polygon-to-requests generator - creates random routing queries within geographic boundaries

import fs from 'fs';
import cla from 'command-line-args';
import clu from 'command-line-usage';
import ansi from 'ansi-escape-sequences';

const url_templates = {
  'route': '/route/v1/driving/{coords}?steps=false&alternatives=false',
  'table': '/table/v1/driving/{coords}'
};

const coord_template = '{lon},{lat}';
const coords_separator = ';';

// Predefined bounding boxes
const bboxes = {
  'planet': [-180., -85., 180., 85.],
  'us': [-127., 24., -67., 48.],
  'germany': [5.9, 47.3, 15.0, 55.1],
  'dc': [-77.138, 38.808, -76.909, 38.974]
};

// Command line argument definitions
const optionsList = [
  {name: 'help', alias: 'h', type: Boolean, description: 'Display this usage guide.', defaultValue: false},
  {name: 'type', alias: 't', type: String, defaultValue: 'route', description: 'Query type: route or table', typeLabel: '{underline route|table}'},
  {name: 'host', type: String, defaultValue: '127.0.0.1', description: 'OSRM server hostname', typeLabel: '{underline hostname}'},
  {name: 'port', alias: 'p', type: Number, defaultValue: 5000, description: 'OSRM server port', typeLabel: '{underline number}'},
  {name: 'coords', alias: 'c', type: Number, defaultValue: 2, description: 'Number of coordinates per query', typeLabel: '{underline number}'},
  {name: 'number', alias: 'n', type: Number, defaultValue: 1000, description: 'Number of queries to generate', typeLabel: '{underline number}'},
  {name: 'sample-size', alias: 's', type: Number, defaultValue: 1000, description: 'Sample size for waypoints mode', typeLabel: '{underline number}'},
  {name: 'bbox', alias: 'b', type: String, defaultOption: true, description: 'Bounding box: planet, us, germany, dc, or path to .poly file', typeLabel: '{underline region|file}'},
  {name: 'axis', alias: 'a', type: String, defaultValue: 'distance', description: 'Query generation mode: distance or waypoints', typeLabel: '{underline distance|waypoints}'}
];

const options = cla(optionsList);

if (options.help) {
  const banner = `\
 ____   ___  __   ______ ____  ____  ____ ____    _________
/ __ \\ / _ \\/ /  / /_/  |/_  // __ \\/ __// __ \\  / / / __(_)
/ /_/ // // / /__/ __/_>  </ // /_/ / _/ / /_/ /_/ /_\\ \\
/ .___/ \\___/____/\\__/_/|_/___|____/___/ \\___\\/___(_)__/
/_/                         |___/`;
  const usage = clu([
    { content: ansi.format(banner, 'green'), raw: true },
    { header: 'Generate OSRM routing/table queries within geographic boundaries', content: 'Outputs queries in CSV format compatible with osrm-runner.js' },
    { header: 'Options', optionList: optionsList },
    { header: 'Examples', content: [
      '$ poly2req.js --bbox germany --number 100',
      '$ poly2req.js -b us -t table -c 10',
      '$ poly2req.js --bbox /path/to/region.poly --axis waypoints'
    ]}
  ]);
  process.stdout.write(`${usage}\n`);
  process.exit(0);
}

// Bounding box coordinates (southwest and northeast corners)
let sw = [Number.MAX_VALUE, Number.MAX_VALUE];
let ne = [-Infinity, -Infinity];

if (options.bbox) {
  if (bboxes[options.bbox]) {
    // Use predefined bounding box
    const bbox = bboxes[options.bbox];
    sw = [bbox[0], bbox[1]];
    ne = [bbox[2], bbox[3]];
  } else {
    // Try to load as .poly file
    const poly_path = options.bbox;
    const poly_data = fs.readFileSync(poly_path, 'utf-8');

    const coordinates = poly_data.split('\n')
      .filter((l) => l != '')
      .slice(2, -2).map((coord_line) => coord_line.split(' ')
        .filter((elem) => elem != ''))
      .map((coord) => [parseFloat(coord[0]), parseFloat(coord[1])]);

    coordinates.forEach((c) => {
      sw[0] = Math.min(sw[0], c[0]);
      sw[1] = Math.min(sw[1], c[1]);
      ne[0] = Math.max(ne[0], c[0]);
      ne[1] = Math.max(ne[1], c[1]);
    });
  }
} else {
  // Default to germany if no bbox specified
  const bbox = bboxes['germany'];
  sw = [bbox[0], bbox[1]];
  ne = [bbox[2], bbox[3]];
}

console.error(sw);
console.error(ne);

// Custom seeded random number generator for reproducible test coordinates
let seed = 0x1337;
function seededRandom(min, max) {
  seed = (seed * 9301 + 49297) % 233280;
  const rnd = seed / 233280;
  return min + rnd * (max - min);
}

// Generate random coordinate within the defined bounding box
function getRandomCoordinate() {
  let lon = seededRandom(sw[0], ne[0]);
  let lat = seededRandom(sw[1], ne[1]);
  return [lon, lat];
}

// Build OSRM API query URL from coordinate array
function makeQuery(coords) {
  const coords_string = coords.map((c) => coord_template.replace('{lon}', c[0]).replace('{lat}', c[1])).join(coords_separator);
  const path = url_templates[options.type].replace('{coords}', coords_string);
  return `http://${options.host}:${options.port}${path}`;
}

// Generate queries based on distance or waypoint count
// Output in CSV format (quoted) for compatibility with osrm-runner.js
if (options.axis == 'distance')
{
  for (let i = 0; i < options.number; ++i)
  {
    let coords = [];
    for (let j = 0; j < options.coords; ++j)
    {
      coords.push(getRandomCoordinate());
    }
    console.log(`"${makeQuery(coords)}"`);
  }
}
else if (options.axis == 'waypoints')
{
  for (let power = 0; power <= 1; ++power)
  {
    for (let factor = 1; factor <= 10; ++factor)
    {
      let num_coords = factor*Math.pow(10, power);
      console.error(num_coords);
      for (let i = 0; i < options['sample-size']; ++i)
      {
        let coords = [];
        for (let j = 0; j < num_coords; ++j)
        {
          coords.push(getRandomCoordinate());
        }
        console.log(`"${makeQuery(coords)}"`);
      }
    }
  }
}
