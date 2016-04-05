#!/usr/bin/env node

'use strict';

let fs = require('fs');

let NUM_REQUEST = 1000;
let NUM_COORDS = 2;
let url_template = "http://127.0.0.1:5000/route/v1/driving/{coords}?steps=false&alternatives=false";
let coord_template = "{lon},{lat}";
let coords_separator = ";";

let monaco_poly_path = process.argv[2];
let poly_data = fs.readFileSync(monaco_poly_path, 'utf-8');

// lets assume there is only one ring
// cut of name and ring number and the two END statements
let coordinates = poly_data.split('\n')
  .filter((l) => l != '')
  .slice(2, -2).map((coord_line) => coord_line.split(' ')
  .filter((elem) => elem != ''))
  .map((coord) => [parseFloat(coord[0]), parseFloat(coord[1])]);

var sw = [Number.MAX_VALUE, Number.MAX_VALUE];
var ne = [Number.MIN_VALUE, Number.MIN_VALUE];

coordinates.forEach((c) => {
  sw[0] = Math.min(sw[0], c[0]);
  sw[1] = Math.min(sw[1], c[1]);
  ne[0] = Math.max(ne[0], c[0]);
  ne[1] = Math.max(ne[1], c[1]);
});

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

for (var i = 0; i < NUM_REQUEST; ++i)
{
  var coords = [];
  for (var j = 0; j < NUM_COORDS; ++j)
  {
    coords.push(getRandomCoordinate());
  }
  let coords_string = coords.map((c) => coord_template.replace("{lon}", c[0]).replace("{lat}", c[1])).join(coords_separator);
  console.log(url_template.replace("{coords}", coords_string));
}

