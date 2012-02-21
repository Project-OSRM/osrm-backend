/*
 *  Open Source Routing Machine (OSRM) - Web (GUI) Interface
 *	Copyright (C) Dennis Luxen, 2011
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU AFFERO General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU Affero General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	or see http://www.gnu.org/licenses/agpl.txt.
 */
 
/**
 * Title: ViaRoute.js
 * Description: JS file for routing with via points
 *
 * @author Dennis Luxen, luxen@kit.edu 
 * @version 0.1 June 2011
 * 
 */
//======================
// OBJECTS

//global timestamp to control frequency of mouse->route distance computations
var timestamp = (new Date()).getTime();
var ISCOMPUTINGDISTANCE = false;
var ISCALCULATINGVIA = false;
//var HASDELETEDTEMPORARYVIAROUTE = false;
var nearestPoint = new OpenLayers.Geometry.Point(1,1);
var nearestSegmentIndex = null;
var viaPointsVector = [];
var p0;

var temporaryViaStyle = { 
	fillColor: "#ffffff",
	strokeColor: "#33ff33", 
	strokeOpacity: 0.6, 
	strokeWidth: 2, 
	pointRadius: 4, 
	pointerEvents: "visiblePainted" 
}; 

var permanentViaStyle = { 
	fillColor: "#ffffff",
	strokeColor: "#0033ff", 
	strokeOpacity: 0.6, 
	strokeWidth: 2, 
	pointRadius: 4, 
	pointerEvents: "visiblePainted" 
}; 

//======================
//FUNCTIONS
function distanceBetweenPoint(x1, y1, x2, y2) {
	var a = x1 - x2;
	var b = y1 - y2;
	return Math.sqrt(a*a + b*b);
}


/* Distance from a point to a line or segment.
x  point's x coord
y  point's y coord
x0 x coord of the line's A point
y0 y coord of the line's A point
x1 x coord of the line's B point
y1 y coord of the line's B point
o specifies if the distance should respect the limits of the segment 
  (overLine = true) or if it should consider the segment as an infinite 
  line (overLine = false), if false returns the distance from the point 
  to the line, otherwise the distance from the point to the segment
*/
function dotLineLength(x, y, x0, y0, x1, y1, o){
    function lineLength(x, y, x0, y0){
        return Math.sqrt((x -= x0) * x + (y -= y0) * y);
    }
    if(o && !(o = function(x, y, x0, y0, x1, y1){
        if(!(x1 - x0)) return {x: x0, y: y};
        else if(!(y1 - y0)) return {x: x, y: y0};
        var left, tg = -1 / ((y1 - y0) / (x1 - x0));
        return {x: left = (x1 * (x * tg - y + y0) + x0 * (x * - tg + y - y1)) / (tg * (x1 - x0) + y0 - y1), y: tg * left - tg * x + y};
    }(x, y, x0, y0, x1, y1), o.x >= Math.min(x0, x1) && o.x <= Math.max(x0, x1) && o.y >= Math.min(y0, y1) && o.y <= Math.max(y0, y1))){
        var l1 = lineLength(x, y, x0, y0), l2 = lineLength(x, y, x1, y1);
        return l1 > l2 ? l2 : l1;
    }
    else {
        var a = y0 - y1, b = x1 - x0, c = x0 * y1 - y0 * x1;
        return Math.abs(a * x + b * y + c) / Math.sqrt(a * a + b * b);
    }
};

function distanceToRoute(pixel) {
	if(!isStartPointSet || !isEndPointSet){
		return;
	}
	if(ISDRAGGING) {
		return;
	}
	
	if(ISCOMPUTINGDISTANCE) {
		return;
	}
	var tempTime = (new Date()).getTime()
	if( tempTime - timestamp < 25) {
		return;
	}
	timestamp = tempTime;
	if(ISCALCULATING) {
		var viaPointFeature = dragLayer.getFeatureBy("name", 'via');
		if(viaPointFeature != null) {
			dragLayer.removeFeatures(viaPointFeature);
			viaPointFeature.destroy;
		}
		return;
	}
	
	if(pixel == null) {
		pixel.x = window.event.clientX;
		pixel.y = window.event.clientY;
	}
	
	var newLonLat = map.getLonLatFromPixel(pixel);
	var minimumDist = 20*Math.pow(2,(19- this.map.getZoom()));
	var minimumDistToVia = 20*Math.pow(2,(19- this.map.getZoom()));
	var indexToInsertViaPoint = 0;
	
	var startFeat = getMarkerByName('start');
	var endFeat = getMarkerByName('end');
	var from = new OpenLayers.LonLat(startFeat.geometry.x,startFeat.geometry.y).transform(EPSG_900913, EPSG_4326);
	var to = new OpenLayers.LonLat(endFeat.geometry.x,endFeat.geometry.y).transform(EPSG_900913, EPSG_4326);

	var mapExtent = map.getExtent();
	for(var i = 0; i < allRoutePoints.length-1; i++) {
		var p1 = allRoutePoints[i];
		var p2 = allRoutePoints[i+1];
		if(p1.x < mapExtent.left || p1.x > mapExtent.right || p1.y < mapExtent.bottom || p1.y > mapExtent.top) {
			if(p2.x < mapExtent.left || p2.x > mapExtent.right || p2.y < mapExtent.bottom || p2.y > mapExtent.top) {
				continue;
			}
		}
		//check if previous segment is closest to via point
		//if yes, then we have passed a via point.
		if(viaPointsVector.length > indexToInsertViaPoint && p0) {
			var viaPoint = new OpenLayers.LonLat(viaPointsVector[indexToInsertViaPoint][1],viaPointsVector[indexToInsertViaPoint][0]).transform(EPSG_4326, EPSG_900913);
			var dist2 = dotLineLength(viaPoint.lon, viaPoint.lat, p1.x, p1.y, p0.x, p0.y, true)
			if( 0 == dist2 ) {
				indexToInsertViaPoint++;
			}
			var tmpDist = distanceBetweenPoint(viaPoint.lat, viaPoint.lon, newLonLat.lat, newLonLat.lon);
			if(tmpDist < minimumDistToVia) {
				minimumDistToVia = tmpDist;
			}
		}
		
		//continue if point out of view
		dist = dotLineLength(newLonLat.lon, newLonLat.lat, p1.x, p1.y, p2.x, p2.y, true);
		if(dist <= minimumDist) {
			nearestSegmentIndex = indexToInsertViaPoint;
			minimumDist = dist;
			var lonDiff = (p2.x - p1.x)
			var m = (p2.y - p1.y) / lonDiff;
			
			var b = p1.y - (m * p1.x);
			var m2 = m*m;
			var x = (m * newLonLat.lat + newLonLat.lon - m * b) / (m2 + 1);
			var y = (m2 * newLonLat.lat + m * newLonLat.lon + b) / (m2 + 1);
			var r = (x - p1.x) / lonDiff;
			if ( r <= 1 && r >= 0 ) {
				nearestPoint.x = x;
				nearestPoint.y = y;
			} else {
				if( r < 0 ) {
					nearestPoint.x = p1.x;
					nearestPoint.y = p1.y;
				} if ( r > 1 ) {
					nearestPoint.x = p2.x;
					nearestPoint.y = p2.y;
				}
			}
			if(p2.x == p1.x) {
				nearestPoint.x = p1.x;
				nearestPoint.y = newLonLat.lat;
			}
		}
	}
	var zoomFactor = Math.pow(2,(19- this.map.getZoom()));
	var viaPointFeature = dragLayer.getFeatureBy("name", 'via');
	if(viaPointFeature != null) {
		dragLayer.removeFeatures(viaPointFeature);
		viaPointFeature.destroy;
	}

	//too close to start or destination?
	if(Math.min(distanceBetweenPoint(startFeat.geometry.x, startFeat.geometry.y, newLonLat.lon, newLonLat.lat),
			distanceBetweenPoint(endFeat.geometry.x, endFeat.geometry.y, newLonLat.lon, newLonLat.lat)) < 10*zoomFactor) {
		ISCOMPUTINGDISTANCE = false;
		return;
	}
	
	//Are we close to the route but sufficiently far away from any existing via point?
	if(minimumDist < 10*zoomFactor && minimumDistToVia > 5*zoomFactor) {
		viaPointFeature = new OpenLayers.Feature.Vector( OpenLayers.Geometry.Polygon.createRegularPolygon( nearestPoint,  (1.5*zoomFactor), 15, 0 ), null, temporaryViaStyle );
		viaPointFeature.name = "via";
		dragLayer.addFeatures( viaPointFeature );
		//console.log('nearestSegmentIndex: ' + nearestSegmentIndex);
	}
	ISCOMPUTINGDISTANCE = false;
	p0 = p1;
}

function computeViaRoute(pixel, isTemporary, skipViaPointsIndex) {
	if(ISCALCULATINGVIA && isTemporary)
		return;
	
	if( nearestSegmentIndex == null ) {
		//console.log('nearestSegmentIndex: ' + nearestSegmentIndex);
		return;
	}
	
	if(isTemporary == null) {
		isTemporary = false;
	}
	
	//So, we jumped all fences
	ISCALCULATINGVIA = true;
		
	var startFeat = getMarkerByName('start');
	var endFeat = getMarkerByName('end');
	var from = new OpenLayers.LonLat(startFeat.geometry.x,startFeat.geometry.y).transform(EPSG_900913, EPSG_4326);
	var to = new OpenLayers.LonLat(endFeat.geometry.x,endFeat.geometry.y).transform(EPSG_900913, EPSG_4326);

	var coordinate = map.getLonLatFromPixel(pixel);
	var via = coordinate.transform(EPSG_900913, EPSG_4326);
	var viaNodeInserted = false;
	var newURL = HOST_ROUTING_URL + "?start="+from.lat + ',' + from.lon + '&dest=' + to.lat + ',' + to.lon;
	newURL += '&geomformat=cmp';
	for(var i = 0; i < viaPointsVector.length; i++) {
		if(i == nearestSegmentIndex) { //insert new via node here
			newURL += '&via=' + via.lat + ',' + via.lon;
			viaNodeInserted = true;
		}
		if(skipViaPointsIndex == i)
			continue;
		newURL += '&via=' + viaPointsVector[i][0] + ',' + viaPointsVector[i][1];
	}
	if(false == viaNodeInserted) {
		newURL += '&via=' + via.lat + ',' + via.lon;
	}
	newURL += '&output=json' + '&z=' + this.map.getZoom();
	if(!isTemporary) {
		newURL += '&instructions=true&jsonp=showResultsRoute';
	} else {
		newURL += '&instructions=false&jsonp=showResultsViaRoute';
	}
	//console.log(newURL);
	var script = document.createElement('script');
	script.id = 'JSONP';
	script.type = 'application/javascript';
	script.src = newURL;
	
	document.body.appendChild(script);
}

function showResultsViaRoute(response) {
		if (response) {
			var viaRouteVector = [];
	    	// now with compression of the route geometry
	        var geometry = decodeRouteGeometry(response.route_geometry, 5);
	       	var lengthOfArray = geometry.length;
	        var points = [];
	        points.length = lengthOfArray;

	        //Create Route Layer for Display
	        for (var i = 0; i < lengthOfArray; i++) {
	            var point = new OpenLayers.Geometry.Point(geometry[i][1], geometry[i][0]).transform(EPSG_4326, EPSG_900913);
	            allRoutePoints.push(point);
				points.push(point);
	            	
				if(i % 1024 == 0 && i>0 || i==lengthOfArray-1){
			/*		var feature1 = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(points), null, {
						strokeColor: "#006600",
						strokeOpacity: 1,
						strokeWidth: 9
					});
			*/		
					var feature = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(points), null, {
						strokeColor: "#aaffaa",
						strokeOpacity: 0.8,
						strokeWidth: 7
					});
			//		viaRouteVector.push(feature1);
					viaRouteVector.push(feature);
					points = [];
					points.push(point);
				}
	        }
			vectorLayerViaRoute.removeFeatures(vectorLayerViaRoute.features);

			for(var i = 0; i< viaRouteVector.length; i++) {
				vectorLayerViaRoute.addFeatures(viaRouteVector[i]);
			}
			viaRouteVector.length = 0;
			ISCALCULATINGVIA = false;
		}    
}    

function paintViaPoints() {
	//remove all previous via points
	var featuresToRemove = [];
	for(var i = 0; i < dragLayer.features.length; i++) { 
		if(dragLayer.features[i].name == "viapoint" || "via" == dragLayer.features[i].name ) {
			 featuresToRemove.push(dragLayer.features[i]);
		}
	}
	dragLayer.removeFeatures(featuresToRemove);
	featuresToRemove.length = 0;
	
	//paint all via points
	var zoomFactor = Math.pow(2,(19- this.map.getZoom()));
	//console.log('painting ' + viaPointsVector.length + ' via points');
	for(var i = 0; i < viaPointsVector.length; i++) {
		//console.log(viaPointsVector[i]);
		var viapoint = new OpenLayers.Geometry.Point(
							viaPointsVector[i][1], 
							viaPointsVector[i][0]);
		viapoint.transform(EPSG_4326, EPSG_900913);
		var viaPointFeature = new OpenLayers.Feature.Vector( OpenLayers.Geometry.Polygon.createRegularPolygon( viapoint,  (1.5*zoomFactor), 20, 0 ), null, permanentViaStyle );
		viaPointFeature.name = "viapoint";
		viaPointFeature.viaIndex = i;
		dragLayer.addFeatures( viaPointFeature )	
	}
}
