/*
 *  Open Source Routing Machine (OSRM) - Web (GUI) Interface
 *	Copyright (C) Pascal Neis, 2011
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
 * Title: Route.js
 * Description: JS file for routing
 *
 * @author Pascal Neis, pascal@neis-one.org 
 * @version 0.2 2011-06-23
 */

//======================
// OBJECTS
//Map
var HOST_ROUTING_URL = 'http://routingdemo.geofabrik.de/route-via/';
var HOST_WEBSITE = 'http://map.project-osrm.org/';//location.host

var ISCALCULATING = false;
var EPSG_4326 = new OpenLayers.Projection("EPSG:4326");
var EPSG_900913 = new OpenLayers.Projection("EPSG:900913");
var allRoutePoints = [];
//======================
// FUNCTIONS
/*
 * routing()-Function to create and send route request
 */
function routing(isDragRoute){
	//Check if startpoint is set		
	if(!isStartPointSet){
		//alert("Please set your Start location first!");
		document.getElementById('information').innerHTML =  '<p class="infoHLRed">Please set your Start location first!</p>';
		return;
	}
	//Check if endpoint is set
	if(!isEndPointSet){ 
		//alert("Please set your Destination first!");
		document.getElementById('information').innerHTML =  '<p class="infoHLRed">Please set your Destination first!</p>';
		return;
	}

	//Check if already a route is planning
	if(!isDragRoute){ ISCALCULATING = false; }
	if(ISCALCULATING){ return; }
	else{ ISCALCULATING = true; }
	
	//Get Coordinates of the Start and Endpoint
	var startFeat = getMarkerByName('start');
	var endFeat = getMarkerByName('end');
	var from = new OpenLayers.LonLat(startFeat.geometry.x,startFeat.geometry.y).transform(new OpenLayers.Projection("EPSG:900913"), new OpenLayers.Projection("EPSG:4326"));
	var to = new OpenLayers.LonLat(endFeat.geometry.x,endFeat.geometry.y).transform(new OpenLayers.Projection("EPSG:900913"), new OpenLayers.Projection("EPSG:4326"));

	//Send Request
	var script = document.createElement('script');
    script.type = 'text/javascript';
    
    var callBackFunction = 'showResultsRoute';
    var instructions = '&geomformat=cmp&instructions=true';
    if(isDragRoute){
       callBackFunction = 'showResultsDragRoute';
       instructions = '&geomformat=cmp&instructions=false';
       document.getElementById('information').innerHTML =  '<p class="infoHL">Release mouse button to get Route Information!</p>(If no Route Summary is diplayed, press the Route!-button)';
    }
    
    script.src = HOST_ROUTING_URL + "&start="+from.lat + ',' + from.lon + '&dest=' + to.lat + ',' + to.lon;
    for(var i = 0; i < viaPointsVector.length; i++) {
		script.src += ('&via=' + viaPointsVector[i][0] + ',' + viaPointsVector[i][1]);
	}
    script.src +='&z='+this.map.getZoom()+'&output=json&jsonp='+callBackFunction+instructions;
    document.body.appendChild(script);
}

/*
 * reroute()-Function
 */
function reroute() {
	if(!isStartPointSet || !isEndPointSet){ return; }	
	routing(false);
}

/*
 * showResultsDragRoute()-Function to show route result for drag-route
 */
function showResultsDragRoute(response) {
    if (response) {
    	//Display Route
    	showRouteGeometry(response);
    }
  	markersLayer.clearMarkers();
    ISCALCULATING = false;
}

/*
 * showResultsRoute()-Function to show route result
 */
function showResultsRoute(response) {
    if (response) {
		//Display Route
		if(document.getElementById("cbNoNames").checked == true){
			showNoNameStreets(response);
		}
		else{
			showRouteGeometry(response);
		}
		
		//Save Via Points that come with route
		var lengthOfArray = response.via_points.length;
		var i = 0;
		viaPointsVector.length = 0;
		viaPointsVector = response.via_points.slice(0);
		paintViaPoints();
 
  		//Create Link of the route
		var startFeat = getMarkerByName('start');
		var endFeat = getMarkerByName('end');
		var from = new OpenLayers.LonLat(startFeat.geometry.x,startFeat.geometry.y).transform(EPSG_900913,EPSG_4326);
		var to = new OpenLayers.LonLat(endFeat.geometry.x,endFeat.geometry.y).transform(EPSG_900913,EPSG_4326);
		var routelink = '<div id="routelink"><input name="routelink" type="submit" title="Get Link" onClick="createShortLink(\''+HOST_WEBSITE+'?fr='+from.lat.toFixed(6)+','+from.lon.toFixed(6)+'&to='+to.lat.toFixed(6)+','+to.lon.toFixed(6);
		for(i = 0; i < viaPointsVector.length; i++) {
				routelink += "&via=" + viaPointsVector[i][0] + "," + viaPointsVector[i][1];
		}
		routelink += '\');" value="Get Link"></div>';

		//Link for the GPX Download 
		var gpxLink = '(<a href="'+HOST_ROUTING_URL+'&start='+from.lat.toFixed(6)+','+from.lon.toFixed(6)+'&dest='+to.lat.toFixed(6)+','+to.lon.toFixed(6)+'&z='+this.map.getZoom();
		for(i = 0; i < viaPointsVector.length; i++) {
				gpxLink += "&via=" + viaPointsVector[i][0] + "," + viaPointsVector[i][1];
		}
		gpxLink += '&output=gpx">Get GPX File</a>)';;

        //Show Route Summary
        var output = '<p class="routeSummaryHL">Some information about your Way <br> from \'<span class="routeSummaryHLlight">'+response.route_summary.start_point+'</span>\' to \'<span class="routeSummaryHLlight">'+response.route_summary.end_point+'</span>\'</p>';
        output += '<p class="routeSummary">Distance: <span class="routeSummarybold">'+response.route_summary.total_distance/1000+' km</span> - Duration: <span class="routeSummarybold">'+secondsToTime(response.route_summary.total_time)+'</span></p><p>'+routelink+'</p><p><span class="routeInstructionsHL">The Route-Instructions:</span> '+gpxLink+'</p>';
        //Show Route Instructions
        output += '<table>';
        lengthOfArray = response.route_instructions.length;

        var rowstyle='routeInstructionsEven';
        var indexPos = response.route_instructions[0][3];
		var point = allRoutePoints[indexPos];
		
		//Hack to suppress output of initial "0m" instruction		
		output += '<tr class="'+rowstyle+'"><td align="right" valign="top"><span class="routeSummarybold">1.</span></td><td class="'+rowstyle+'"><a href="#" class="nolinkStyle" onclick="setMapCenter(new OpenLayers.LonLat('+point.x+','+point.y+'));">'+response.route_instructions[0][0]+' on '+response.route_instructions[0][1];
		if(response.route_instructions[0][2] != 0)
			output += ' for '+getDistanceWithUnit(response.route_instructions[0][2]);
	   	output += '</a></td></tr>';
 
		for (i = 1; i < lengthOfArray; i++) {
			//console.log(response.route_instructions[i]);
        	//odd or even ?
        	if(i%2==0){ 
 				rowstyle='routeInstructionsEven'; 
 			} else {
 				rowstyle='routeInstructionsOdd';
 			}	
			
            indexPos = response.route_instructions[i][3];
			//console.log('setting : ' + response.route_instructions[i] + ' at ' + allRoutePoints[indexPos]);
			point = allRoutePoints[indexPos];
 			       	
            output += '<tr class="'+rowstyle+'"><td align="right" valign="top"><span class="routeSummarybold">'+(i+1)+'.</span></td><td class="'+rowstyle+'"><a href="#" class="nolinkStyle" onclick="setMapCenter(new OpenLayers.LonLat('+point.x+','+point.y+'));">'+response.route_instructions[i][0]+' on '+response.route_instructions[i][1]+' for '+getDistanceWithUnit(response.route_instructions[i][2])+'</a></td></tr>';
        }
		var rowstyle='routeInstructionsOdd';
       	if(i%2==0){ rowstyle='routeInstructionsEven'; }
		if( lengthOfArray > 0) {
	        var point = allRoutePoints[allRoutePoints.length-1];
	        output += '<tr class="'+rowstyle+'"><td align="right" valign="top"><span class="routeSummarybold">'+(i+1)+'.</span></td><td class="'+rowstyle+'"><a href="#" class="nolinkStyle" onclick="setMapCenter(new OpenLayers.LonLat('+point.x+','+point.y+'));">You have reached your destination</a></td></tr>';
        }
		output += '</table>';
        //alert(vectorLayerRoute.features[0].geometry.getVertices());
        
        document.getElementById('information').innerHTML = output;
	    ISCALCULATING = false;
		ISCALCULATINGVIA = false;
    }
}

/*
 * showRouteGeometry()-Function to show route result
 */
function showRouteGeometry(response) {
	if (response) {
	 	allRoutePoints.length = 0;
    	// now with compression of the route geometry
        var geometry = decodeRouteGeometry(response.route_geometry, 5);
       	var lengthOfArray = geometry.length;
        var points = [];
        points.length = lengthOfArray;
        var points = [];
 		//delete any previously displayed via route
		vectorLayerViaRoute.removeFeatures(vectorLayerViaRoute.features);
		vectorLayerRoute.removeAllFeatures();
 
        //Create Route Layer for Display
        for (var i = 0; i < lengthOfArray; i++) {
            var point = new OpenLayers.Geometry.Point(geometry[i][1], geometry[i][0]).transform(EPSG_4326, EPSG_900913);
            allRoutePoints.push(point);
			points.push(point);
            	
			if(i % 1024 == 0 && i>0 || i==lengthOfArray-1){
/*	 			var feature2 = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(points), null, {
					strokeColor: "#000066",
					strokeOpacity: 1,
					strokeWidth: 9
				});

	 			var feature1 = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(points), null, {
					strokeColor: "#ffffff",
					strokeOpacity: 0.75,
					strokeWidth: 7
				});
*/

	 			var feature = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(points), null, {
					strokeColor: "#0033ff",
					strokeOpacity: 0.7,
					strokeWidth: 5
				});
			//	vectorLayerRoute.addFeatures([feature2]);
			//	vectorLayerRoute.addFeatures([feature1]);
				vectorLayerRoute.addFeatures([feature]);
				points = [];
				points.push(point);
			}
        }
   }
}

/*
 * showNoNameStreets()-Function to show route result
 */
function showNoNameStreets(response) {
    if (response) {
    	allRoutePoints.length = 0;
    	vectorLayerRoute.removeFeatures(vectorLayerRoute.features);
      	// now with compression of the route geometry
        var geometry = decodeRouteGeometry(response.route_geometry, 5);
       	var lengthOfArray = geometry.length;
        var points = [];
        points.length = lengthOfArray;
        
        //Check if a instruction has no name !
        var colors = [];
        if(true){
        	var instrucLength = response.route_instructions.length;
        	for (var i = 0; i < instrucLength; i++) {
        		var indexPos = response.route_instructions[i][3];
        		var streetName = response.route_instructions[i][1];
				if(streetName == ''){ colors[indexPos] = "#FF00FF"; }
				else{ colors[indexPos] = "#0033ff"; }
        	}
        }

 		//delete any previously displayed via route
		vectorLayerViaRoute.removeFeatures(vectorLayerViaRoute.features);
		vectorLayerRoute.removeAllFeatures();
        
        //Create Route Layer for Display
        var color = "#0033ff";
        var changeColor = false;
        for (var i = 0; i < lengthOfArray; i++) {
            var point = new OpenLayers.Geometry.Point(geometry[i][1], geometry[i][0]).transform(EPSG_4326, EPSG_900913);
            points.push(point);
            allRoutePoints.push(point);
				
            if(colors[i] != undefined){ changeColor=true;}
            	
			if(i % 1024 == 0 && i>0 || i==lengthOfArray-1 || changeColor){
				var feature = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.LineString(points), null, {
					strokeColor: color,
					strokeOpacity: 0.7,
					strokeWidth: 5
				});
				vectorLayerRoute.addFeatures([feature]);
				points = [];
				points.push(point);
				
				if(colors[i] != undefined){ color = colors[i]; }
				changeColor=false;
			}
        }
   }
}

/*
 * secondsToTime()-Function to transform seconds to a time string
 */
function secondsToTime(seconds){
   seconds = parseInt(seconds);
   minutes = parseInt(seconds/60);
   seconds = seconds%60;
   hours = parseInt(minutes/60);
   minutes = minutes%60;
   //return stunden+':'+minuten+':'+seconds;
   if(hours==0){
   	return minutes+' min(s)';
   }
   else{
   	return hours+' h '+minutes+' min(s)';
   }
}

/*
 * getDistanceWithUnit()-Function to return a distance with units
 */
function getDistanceWithUnit(distance){
	distance = parseInt(distance);
	if(distance >= 1000){ return (parseInt(distance/1000))+' km'; }
	else{ return distance+' m'; }
}

/*
 * setMapCenter()-Function to add a marker and center the map
 */
function setMapCenter(lonlat){
	//lonlat.transform(new OpenLayers.Projection("EPSG:4326"), new OpenLayers.Projection("EPSG:900913"));
	//console.log('zooming to :' + lonlat);
	
	//Add Marker
	markersLayer.clearMarkers();
	var size = new OpenLayers.Size(21,25);
    var offset = new OpenLayers.Pixel(-(size.w/2), -size.h);
    var icon = new OpenLayers.Icon('img/marker.png',size,offset);
    markersLayer.addMarker(new OpenLayers.Marker(lonlat,icon));

	//Hack - FIXME !
	map.setCenter(new OpenLayers.LonLat(lonlat.lon-200, lonlat.lat), 17);
}

/*
 * decodeRouteGeometry()-Function to decode encoded Route Geometry
 */
function decodeRouteGeometry(encoded, precision) {
	precision = Math.pow(10, -precision);
	var len = encoded.length, index=0, lat=0, lng = 0, array = [];
	while (index < len) {
		var b, shift = 0, result = 0;
		do {
			b = encoded.charCodeAt(index++) - 63;
			result |= (b & 0x1f) << shift;
			shift += 5;
		} while (b >= 0x20);
		var dlat = ((result & 1) ? ~(result >> 1) : (result >> 1));
		lat += dlat;
		shift = 0;
		result = 0;
		do {
			b = encoded.charCodeAt(index++) - 63;
			result |= (b & 0x1f) << shift;
			shift += 5;
		} while (b >= 0x20);
		var dlng = ((result & 1) ? ~(result >> 1) : (result >> 1));
		lng += dlng;
		array.push([lat * precision, lng * precision]);
	}
	return array;
}

/*
 * createShortLink()-Function to "create" shortlink of a route
 */
function createShortLink(str){
	var script = document.createElement('script');
    script.type = 'text/javascript';
    var callBackFunction = 'showRouteLink';
    script.src = 'http://map.project-osrm.org/shorten/'+str+'&jsonp=showRouteLink';  	
    document.body.appendChild(script);
}

/*
 * showRouteLink()-Function
 */
function showRouteLink(response){
	document.getElementById('routelink').innerHTML = '<span class="routeSummarybold"> >> Your ShortLink:</span> <a href="'+response.ShortURL+'">'+response.ShortURL+'</a>';
}
