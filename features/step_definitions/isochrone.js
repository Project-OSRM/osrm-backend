// Step definitions for validating Isochrone service responses.
import assert from 'node:assert';

import { Then, When } from '@cucumber/cucumber';

function parseContours(contours) {
  const values = contours.split(',').map(Number);
  values.forEach((value) => {
    assert.ok(Number.isFinite(value), `invalid contour in test: ${contours}`);
  });
  return values;
}

async function requestIsochrone(world, nodeName, contours, options = {}) {
  const node = world.findNodeByName(nodeName);
  assert.ok(node, `unknown isochrone input node "${nodeName}"`);

  const parameters = Object.assign({}, world.queryParams, options, { contours });
  const { response, body } = await new Promise((resolve, reject) => {
    world.requestIsochrone(node, parameters, (err, response, body) => {
      if (err) return reject(err);
      resolve({ response, body });
    });
  });

  let json;
  try {
    json = JSON.parse(body);
  } catch {
    throw new Error(`isochrone response is not JSON: ${body}`);
  }
  return { response, json, body };
}

function assertPosition(position) {
  assert.ok(Array.isArray(position) && position.length >= 2, 'expected a GeoJSON position');
  assert.ok(Number.isFinite(position[0]), 'expected finite longitude');
  assert.ok(Number.isFinite(position[1]), 'expected finite latitude');
  assert.ok(position[0] >= -180 && position[0] <= 180, 'longitude is outside [-180, 180]');
  assert.ok(position[1] >= -90 && position[1] <= 90, 'latitude is outside [-90, 90]');
}

function assertRing(ring) {
  assert.ok(Array.isArray(ring) && ring.length >= 4, 'expected a non-empty linear ring');
  ring.forEach(assertPosition);
  assert.deepStrictEqual(ring[0], ring[ring.length - 1], 'expected a closed linear ring');
}

function assertGeometry(geometry, geometryType) {
  assert.strictEqual(geometry.type, geometryType);
  assert.ok(Array.isArray(geometry.coordinates), 'expected geometry coordinates');
  assert.ok(geometry.coordinates.length > 0, 'expected non-empty geometry coordinates');

  if (geometryType === 'MultiPolygon') {
    geometry.coordinates.forEach((polygon) => {
      assert.ok(Array.isArray(polygon) && polygon.length > 0, 'expected a polygon with a ring');
      polygon.forEach(assertRing);
    });
  } else {
    geometry.coordinates.forEach(assertRing);
  }
}

function pointIsOnSegment(point, start, end) {
  const [x, y] = point;
  const [startX, startY] = start;
  const [endX, endY] = end;
  const squaredLength = (endX - startX) ** 2 + (endY - startY) ** 2;
  if (squaredLength <= 1e-24)
    return Math.abs(x - startX) <= 1e-12 && Math.abs(y - startY) <= 1e-12;

  const crossProduct = (x - startX) * (endY - startY) - (y - startY) * (endX - startX);
  if (Math.abs(crossProduct) > 1e-12)
    return false;

  const dotProduct = (x - startX) * (endX - startX) + (y - startY) * (endY - startY);
  if (dotProduct < 0)
    return false;

  return dotProduct <= squaredLength;
}

function pointIsInRing(point, ring) {
  let inside = false;
  for (let current = 0, previous = ring.length - 1; current < ring.length; previous = current++) {
    const currentPoint = ring[current];
    const previousPoint = ring[previous];
    if (pointIsOnSegment(point, previousPoint, currentPoint))
      return true;

    const crossesLatitude = (currentPoint[1] > point[1]) !== (previousPoint[1] > point[1]);
    const intersectionLongitude =
      ((previousPoint[0] - currentPoint[0]) * (point[1] - currentPoint[1])) /
        (previousPoint[1] - currentPoint[1]) +
      currentPoint[0];
    if (crossesLatitude && point[0] < intersectionLongitude)
      inside = !inside;
  }
  return inside;
}

function geometryContainsPoint(geometry, point) {
  return geometry.coordinates.some((polygon) => {
    const [outer, ...holes] = polygon;
    return pointIsInRing(point, outer) && !holes.some((hole) => pointIsInRing(point, hole));
  });
}

function assertFeatureCollection(result, featureCount, geometryType) {
  assert.strictEqual(result.response.statusCode, 200, `unexpected response: ${result.body}`);
  assert.strictEqual(result.json.code, 'Ok', `unexpected response: ${result.body}`);
  assert.strictEqual(result.json.type, 'FeatureCollection');
  assert.ok(!Object.hasOwn(result.json, 'weight_name'),
    'isochrone responses must not expose primary routing weight metadata');
  assert.ok(Array.isArray(result.json.features), 'expected GeoJSON features');
  assert.strictEqual(result.json.features.length, featureCount);

  result.json.features.forEach((feature) => {
    assert.strictEqual(feature.type, 'Feature');
    assert.ok(feature.properties, 'expected feature properties');
    assert.ok(Number.isFinite(feature.properties.contour), 'expected numeric contour property');
    assert.ok(Number.isFinite(feature.properties.effective_contour),
      'expected numeric effective contour property');
    assert.ok(feature.properties.effective_contour <= feature.properties.contour,
      'effective contour must not exceed the requested contour');
    assertGeometry(feature.geometry, geometryType);
  });
}

When(/^I request an isochrone from "([a-z0-9])" with contours "([^"]+)"$/, async function (node, contours) {
  await this.reprocessAndLoadData();
  this.isochroneResponse = await requestIsochrone(this, node, contours);
});

When(/^I request outbound and inbound isochrones from "([a-z0-9])" with contour "([^"]+)"$/, async function (node, contour) {
  await this.reprocessAndLoadData();
  this.outboundIsochroneResponse = await requestIsochrone(this, node, contour, {
    direction: 'outbound',
  });
  this.inboundIsochroneResponse = await requestIsochrone(this, node, contour, {
    direction: 'inbound',
  });
});

Then(/^the isochrone response should be a GeoJSON FeatureCollection with "(\d+)" "(MultiPolygon|MultiLineString)" features$/, function (featureCount, geometryType) {
  assert.ok(this.isochroneResponse, 'no isochrone response was recorded');
  assertFeatureCollection(this.isochroneResponse, Number(featureCount), geometryType);
});

Then(/^the isochrone contours should be "([^"]+)"$/, function (contours) {
  assert.ok(this.isochroneResponse, 'no isochrone response was recorded');
  assert.deepStrictEqual(
    this.isochroneResponse.json.features.map((feature) => feature.properties.contour),
    parseContours(contours),
  );
});

Then(/^the isochrone response should have "(\d+)" waypoint$/, function (waypointCount) {
  assert.ok(this.isochroneResponse, 'no isochrone response was recorded');
  assert.ok(Array.isArray(this.isochroneResponse.json.waypoints), 'expected waypoints');
  assert.strictEqual(this.isochroneResponse.json.waypoints.length, Number(waypointCount));
});

Then(/^the isochrone response should omit waypoints$/, function () {
  assert.ok(this.isochroneResponse, 'no isochrone response was recorded');
  assert.ok(!Object.hasOwn(this.isochroneResponse.json, 'waypoints'));
});

Then(/^the isochrone response should contain an empty area$/, function () {
  assert.ok(this.isochroneResponse, 'no isochrone response was recorded');
  const { response, json, body } = this.isochroneResponse;
  assert.strictEqual(response.statusCode, 200, `unexpected response: ${body}`);
  assert.strictEqual(json.code, 'Ok', `unexpected response: ${body}`);
  assert.strictEqual(json.type, 'FeatureCollection');
  assert.ok(Array.isArray(json.features), 'expected GeoJSON features');
  assert.strictEqual(json.features.length, 1);
  assert.strictEqual(json.features[0].geometry.type, 'MultiPolygon');
  assert.deepStrictEqual(json.features[0].geometry.coordinates, []);
});

Then(/^both directional isochrone responses should be GeoJSON FeatureCollections$/, function () {
  assert.ok(this.outboundIsochroneResponse, 'no outbound isochrone response was recorded');
  assert.ok(this.inboundIsochroneResponse, 'no inbound isochrone response was recorded');
  assertFeatureCollection(this.outboundIsochroneResponse, 1, 'MultiPolygon');
  assertFeatureCollection(this.inboundIsochroneResponse, 1, 'MultiPolygon');
});

Then(/^the outbound and inbound isochrone geometries should differ$/, function () {
  assert.notDeepStrictEqual(
    this.outboundIsochroneResponse.json.features.map((feature) => feature.geometry),
    this.inboundIsochroneResponse.json.features.map((feature) => feature.geometry),
  );
});

Then(/^the outbound isochrone should contain "([a-z0-9])" and the inbound isochrone should not$/, function (nodeName) {
  const node = this.findNodeByName(nodeName);
  assert.ok(node, `unknown expected node "${nodeName}"`);
  const point = [node.lon, node.lat];
  const outboundGeometry = this.outboundIsochroneResponse.json.features[0].geometry;
  const inboundGeometry = this.inboundIsochroneResponse.json.features[0].geometry;

  assert.ok(geometryContainsPoint(outboundGeometry, point),
    `expected outbound isochrone to contain ${nodeName}`);
  assert.ok(!geometryContainsPoint(inboundGeometry, point),
    `expected inbound isochrone not to contain ${nodeName}`);
});

Then(/^the isochrone should contain "([a-z0-9])" and not contain "([a-z0-9])"$/, function (includedName, excludedName) {
  assert.ok(this.isochroneResponse, 'no isochrone response was recorded');
  const included = this.findNodeByName(includedName);
  const excluded = this.findNodeByName(excludedName);
  assert.ok(included, `unknown expected node "${includedName}"`);
  assert.ok(excluded, `unknown excluded node "${excludedName}"`);

  const geometry = this.isochroneResponse.json.features[0].geometry;
  assert.ok(geometryContainsPoint(geometry, [included.lon, included.lat]),
    `expected isochrone to contain ${includedName}`);
  assert.ok(!geometryContainsPoint(geometry, [excluded.lon, excluded.lat]),
    `expected isochrone not to contain ${excludedName}`);
});

Then(/^the isochrone HTTP status should be (\d+)$/, function (status) {
  assert.strictEqual(this.response.statusCode, Number(status), `unexpected response: ${this.body}`);
});
