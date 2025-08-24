#!/usr/bin/env node

'use strict';

let fs = require('fs');  // Node 4.x required!

let VERSION = "route_5.0";
let SAMPLE_SIZE = 20;
let NUM_REQUEST = 100;
let NUM_COORDS = 2;
let PORT = 5000;
let url_templates = {
  "route_5.0": "http://127.0.0.1:{port}/route/v1/driving/{coords}?steps=false&alternatives=false",
  "route_4.9": "http://127.0.0.1:{port}/viaroute?{coords}&instructions=false&alt=false",
  "table_5.0": "http://127.0.0.1:{port}/table/v1/driving/{coords}",
  "table_4.9": "http://127.0.0.1:{port}/table?{coords}"
};

let coord_templates =  {
  "route_5.0": "{lon},{lat}",
  "route_4.9": "loc={lat},{lon}",
  "table_5.0": "{lon},{lat}",
  "table_4.9": "loc={lat},{lon}"
};

let coords_separators = {
  "route_5.0": ";",
  "route_4.9": "&",
  "table_5.0": ";",
  "table_4.9": "&"
};
var axis = "distance";

var sw = [Number.MAX_VALUE, Number.MAX_VALUE];
var ne = [Number.MIN_VALUE, Number.MIN_VALUE];

if (process.argv.length > 2 && process.argv[2] == "planet")
{
  sw = [-180., -85.];
  ne = [180., 85.];
}
if (process.argv.length > 2 && process.argv[2] == "us")
{
  sw = [-127., 24.];
  ne = [-67., 48.];
}
else if (process.argv.length > 2 && process.argv[2] == "dc")
{
  sw = [-77.138, 38.808];
  ne = [-76.909, 38.974];
}
else if (process.argv.length > 2)
{
  let poly_path = process.argv[2];
  let poly_data = fs.readFileSync(poly_path, 'utf-8');

  // lets assume there is only one ring
  // cut of name and ring number and the two END statements
  let coordinates = poly_data.split('\n')
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

if (process.argv.length > 3)
{
  axis = process.argv[3];
}

console.error(sw);
console.error(ne);

// Yes this an own seeded random number generator because its only a few lines
var seed = 0x1337;
function seededRandom(min, max) {
  seed = (seed * 9301 + 49297) % 233280;
  var rnd = seed / 233280;
  return min + rnd * (max - min);
}

function getRandomCoordinate() {
  let lon = seededRandom(sw[0], ne[0]);
  let lat = seededRandom(sw[1], ne[1]);
  return [lon, lat];
}

function makeQuery(coords) {
  let coords_string = coords.map((c) => coord_templates[VERSION].replace("{lon}", c[0]).replace("{lat}", c[1])).join(coords_separators[VERSION]);
  return url_templates[VERSION].replace("{coords}", coords_string).replace("{port}", PORT);
}

if (axis == "distance")
{
  for (var i = 0; i < NUM_REQUEST; ++i)
  {
    var coords = [];
    for (var j = 0; j < NUM_COORDS; ++j)
    {
      coords.push(getRandomCoordinate());
    }
    console.log(makeQuery(coords));
  }
}
else if (axis == "waypoints")
{
  for (var power = 0; power <= 1; ++power)
  {
    for (var factor = 1; factor <= 10; ++factor)
    {
      let num_coords = factor*Math.pow(10, power);
      console.error(num_coords);
      for (var i = 0; i < SAMPLE_SIZE; ++i)
      {
        var coords = [];
        for (var j = 0; j < num_coords; ++j)
        {
          coords.push(getRandomCoordinate());
        }
        console.log(makeQuery(coords));
      }
    }
  }
}

