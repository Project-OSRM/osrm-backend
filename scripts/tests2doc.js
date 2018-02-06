"use strict"

const fs = require('fs');
const path = require('path');
const d3 = require('d3-queue');


var mapnik = require('mapnik');

mapnik.register_default_input_plugins();
mapnik.register_system_fonts()

//console.log(mapnik.fonts());

const find = (dir) =>
  fs.readdirSync(dir)
    .reduce((files, file) =>
      fs.statSync(path.join(dir, file)).isDirectory() ?
        files.concat(find(path.join(dir, file))) :
        files.concat(path.join(dir, file)),
      []);

function makemappng(basefile, routefile, callback) {

var stylesheet = `
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE Map[]>
<Map srs="+init=epsg:3857" buffer-size="8">

<Style name="testmap" filter-mode="all">
  <Rule>
    <Filter>[highway] = 'motorway'</Filter>
    <LineSymbolizer stroke-width="12" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#aacc77" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-10'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='10'>'highway=' + [highway]</TextSymbolizer>
  </Rule>
  <Rule>
    <Filter>[highway] = 'motorway_link'</Filter>
    <LineSymbolizer stroke-width="12" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#77aacc" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-10'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='10'>'highway=' + [highway]</TextSymbolizer>
  </Rule>

  <Rule>
    <Filter>[highway] = 'trunk'</Filter>
    <LineSymbolizer stroke-width="12" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#ccaa77" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-10'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='10'>'highway=' + [highway]</TextSymbolizer>
  </Rule>
  <Rule>
    <Filter>[highway] = 'trunk_link'</Filter>
    <LineSymbolizer stroke-width="12" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#aa77cc" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-10'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='10'>'highway=' + [highway]</TextSymbolizer>
  </Rule>

  <Rule>
    <Filter>[highway] = 'primary'</Filter>
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="8" stroke-linejoin="round" stroke-linecap="round" stroke="#77ccaa" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-8'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='8'>'highway=' + [highway]</TextSymbolizer>
  </Rule>
  <Rule>
    <Filter>[highway] = 'primary_link'</Filter>
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="8" stroke-linejoin="round" stroke-linecap="round" stroke="#aa77cc" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-8'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='8'>'highway=' + [highway]</TextSymbolizer>
  </Rule>

  <Rule>
    <Filter>[highway] = 'secondary' or [highway] = 'secondary_link' or [highway] = 'tertiary' or [highway] = 'tertiary_link' or [highway] = 'residential' or [highway] = 'service' or [highway] = 'living_street'</Filter>
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="8" stroke-linejoin="round" stroke-linecap="round" stroke="#77bb77" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-8'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='8'>'highway=' + [highway]</TextSymbolizer>
  </Rule>

  <Rule>
    <Filter>[route] = 'ferry'</Filter>
    <LineSymbolizer stroke-width="10" stroke-linejoin="round" stroke-linecap="round" stroke="#bbbbbb" />
    <LineSymbolizer stroke-width="8" stroke-linejoin="round" stroke-linecap="round" stroke="#7777bb" />
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='-8'>'name=' + [name]</TextSymbolizer>
    <TextSymbolizer face-name="Arial Regular" size="10" fill="black" placement="line" allow-overlap="false" dy='8'>'route=' + [route]</TextSymbolizer>
  </Rule>

</Style>

<Layer name="testmap" srs="+init=epsg:4326">
    <StyleName>testmap</StyleName>
    <Datasource>
       <Parameter name="type">geojson</Parameter>
       <Parameter name="file">${basefile}</Parameter>
    </Datasource>
</Layer>

<Style name="testroute-line" filter-mode="all">
  <Rule>
    <LineSymbolizer stroke-width="5" stroke-linejoin="round" stroke-linecap="round" stroke="#000077" offset="6"/>
    <LineSymbolizer stroke-width="3" stroke-linejoin="round" stroke-linecap="round" stroke="#0000ff" offset="6"/>
  </Rule>
</Style>
<Style name="testroute-markers" filter-mode="all">
  <Rule>
    <Filter>[type] = 'startpoint'</Filter>
    <MarkersSymbolizer fill="green" width="20" height="20" stroke="green"/>
  </Rule>
  <Rule>
    <Filter>[type] = 'endpoint'</Filter>
    <MarkersSymbolizer fill="red" width="20" height="20" stroke="red"/>
  </Rule>
</Style>
<Layer name="testroute" srs="+init=epsg:4326">
    <StyleName>testroute-line</StyleName>
    <StyleName>testroute-markers</StyleName>
    <Datasource>
       <Parameter name="type">geojson</Parameter>
       <Parameter name="file">${routefile}</Parameter>
    </Datasource>
</Layer>
</Map>`;

    var map = new mapnik.Map(300, 200);
    map.fromStringSync(stylesheet,{strict:true});
    map.zoomAll();

    var extent = map.extent;
    extent[0] = extent[0] - Math.abs(extent[0]) * 0.00001;
    extent[1] = extent[1] - Math.abs(extent[1]) * 0.00001;

    extent[2] = extent[2] + Math.abs(extent[2]) * 0.00001;
    extent[3] = extent[3] + Math.abs(extent[3]) * 0.00001;
    map.zoomToBox(extent);

    var buffer = new mapnik.Image(300,200)
    map.render(buffer, {}, (err,image) => {
      callback(image.encodeSync('png'));
    });
}

var report = "";

var toc = [];

find('test/cache').filter((f) => f.match(/[0-9]+_results.json$/)).forEach((f) => {
  var files = f.match(/(.*?)_([0-9]+)_results.json$/,f);

  var results = JSON.parse(fs.readFileSync(f));

  // Generate map image
  var imagefile = `${files[1]}_${files[2]}.png`.replace(/\//g,'_');
  /*
  var png = makemappng(`${files[1]}.geojson`, `${files[1]}_${files[2]}_shape.geojson`, (png) => {
    fs.writeFileSync(`report/${imagefile}`,png);
  });
  */

  toc.push({ title: `${results.feature} - ${results.scenario}`, link: imagefile });

  if (typeof results.got.turns === 'object')
      results.got.turns = results.got.turns.join(',');

  if (typeof results.expected.turns === 'string')
      results.expected.turns = results.expected.turns.replace(/slight /g,'').replace(/sharp /g, '');
  if (typeof results.got.turns === 'string')
      results.got.turns = results.got.turns.replace(/slight /g,'').replace(/sharp /g, '');

  report += `<div class='scenario ${results.got.turns == results.expected.turns ? 'ok' : 'error'}'>
  <h2><a name="${imagefile}">${results.feature} - ${results.scenario}</a></h2>
<table class="row">
  <tr>
    <td>
  <img src="${imagefile}"/>
    </td>
  <td>
  <table class="results">
      <tr><th/><th style='text-align: left'>Route</th><th style='text-align: left'>Turns</th></tr>
      <tr><th style='text-align: right'>OSRM</th><td>${results.expected.route}</td><td>${results.expected.turns}</td></tr>
      <tr><th style='text-align: right'>Valhalla</th><td>${results.got.route}</td><td class='${results.got.turns == results.expected.turns ? 'ok' : 'error'}'>${results.got.turns}</td></tr>
  </table>
</td></tr></table>
</div>
<hr/>
`;

  // Generate HTML table

});

console.log(`
<!DOCYPE html>
<html>
<head>
  <style>
    body { font-family: sans-serif; }
    .results td { padding-left: 1em; font-family: monospace; }
    .results th { padding-left: 1em; }
    .scenario.error { background: #fbb; }
    .error { color: red; }
  </style>
  <body>
Notes: OSRM "slight" and "sharp" indicators have been removed for comparison purposes.
<hr/>
`);
toc.forEach(r => {
  console.log(`<a href='#${r.link}'>${r.title}</a><br/>`);
});
console.log(report);
console.log(`
</body>
</html>
`);
