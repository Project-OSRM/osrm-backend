/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
*/

// OSRM routing routines
// [management of routing/direction requests and processing of responses]
// [TODO: major refactoring scheduled]

// some variables
OSRM.GLOBALS.route = null;
OSRM.GLOBALS.markers = null;

OSRM.CONSTANTS.NO_DESCRIPTION = 0;
OSRM.CONSTANTS.FULL_DESCRIPTION = 1;

OSRM.G.dragging = null;
OSRM.GLOBALS.dragid = null;
OSRM.GLOBALS.pending = false;
OSRM.GLOBALS.pendingTimer = null;


// init routing data structures
function initRouting() {
	OSRM.G.route = new OSRM.Route();
	OSRM.G.markers = new OSRM.Markers();
}


// -- JSONP processing -- 

// process JSONP response of routing server
function timeoutRouteSimple() {
	showNoRouteGeometry();
	showNoRouteDescription();
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";
}
function timeoutRoute() {
	showNoRouteGeometry();
	OSRM.G.route.hideUnnamedRoute();
	showNoRouteDescription();
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";	
}
function showRouteSimple(response) {
 	if(!response)
 		return;
 	
 	if( !OSRM.G.dragging )		// prevent simple routing when no longer dragging
 		return;
 	
	if( response.status == 207) {
		showNoRouteGeometry();
		showNoRouteDescription();
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED")+".<p>";
	} else {
		showRouteGeometry(response);
		showRouteDescriptionSimple(response);
	}
	updateHints(response);

//	// TODO: hack to process final drag event, if it was fenced, but we are still dragging (alternative approach)
//	if(OSRM.G.pending) {
//		clearTimeout(OSRM.G.pendingTimer);
//		OSRM.G.pendingTimer = setTimeout(timeoutDrag,100);		
//	}
}
function showRoute(response) {
	if(!response)
		return;
	
	if(response.status == 207) {
		showNoRouteGeometry();
		OSRM.G.route.hideUnnamedRoute();
		showNoRouteDescription();
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_ROUTE_FOUND")+".<p>";
	} else {
		showRouteGeometry(response);
		showRouteNonames(response);
		showRouteDescription(response);
		snapRoute();
	}
	updateHints(response);
}


// show route geometry
function showNoRouteGeometry() {
	var positions = [];
	for(var i=0; i<OSRM.G.markers.route.length;i++)
			positions.push( OSRM.G.markers.route[i].getPosition() );

	OSRM.G.route.showRoute(positions, OSRM.Route.NOROUTE);
}
function showRouteGeometry(response) {
	OSRM.G.via_points = response.via_points.slice(0);

	var geometry = decodeRouteGeometry(response.route_geometry, 5);

	var points = [];
	for( var i=0; i < geometry.length; i++) {
		points.push( new L.LatLng(geometry[i][0], geometry[i][1]) );
	}
	OSRM.G.route.showRoute(points, OSRM.Route.ROUTE);
}


// route description display (and helper functions)
function onClickRouteDescription(geometry_index) {
	var positions = OSRM.G.route.getPositions();

	OSRM.G.markers.highlight.setPosition( positions[geometry_index] );
	OSRM.G.markers.highlight.show();
	OSRM.G.markers.highlight.centerView(OSRM.DEFAULTS.HIGHLIGHT_ZOOM_LEVEL);	
}
function onClickCreateShortcut(src){
	src += '&z='+ OSRM.G.map.getZoom() + '&center=' + OSRM.G.map.getCenter().lat + ',' + OSRM.G.map.getCenter().lng;
	OSRM.JSONP.call(OSRM.DEFAULTS.HOST_SHORTENER_URL+src, showRouteLink, showRouteLink_TimeOut, 2000, 'shortener');
	document.getElementById('route-prelink').innerHTML = '['+OSRM.loc("GENERATE_LINK_TO_ROUTE")+']';
}
function showRouteLink(response){
	document.getElementById('route-prelink').innerHTML = '[<a id="gpx-link" class = "text-selectable" href="' +response.ShortURL+ '">'+response.ShortURL+'</a>]';
}
function showRouteLink_TimeOut(){
	document.getElementById('route-prelink').innerHTML = '['+OSRM.loc("LINK_TO_ROUTE_TIMEOUT")+']';
}
function showRouteDescription(response) {
	// compute query string
	var query_string = '?rebuild=1';
	for(var i=0; i<OSRM.G.markers.route.length; i++)
		query_string += '&loc=' + OSRM.G.markers.route[i].getLat() + ',' + OSRM.G.markers.route[i].getLng(); 
 						
	// create link to the route
	var route_link ='<span class="route-summary" id="route-prelink">[<a id="gpx-link" onclick="onClickCreateShortcut(\'' + OSRM.DEFAULTS.WEBSITE_URL + query_string + '\')">'+OSRM.loc("GET_LINK_TO_ROUTE")+'</a>]</span>';

	// create GPX link
	var gpx_link = '<span class="route-summary">[<a id="gpx-link" onClick="document.location.href=\'' + OSRM.DEFAULTS.HOST_ROUTING_URL + query_string + '&output=gpx\';">'+OSRM.loc("GPX_FILE")+'</a>]</span>';
		
	// create route description
	var route_desc = "";
	route_desc += '<table class="results-table">';

	for(var i=0; i < response.route_instructions.length; i++){
		//odd or even ?
		var rowstyle='results-odd';
		if(i%2==0) { rowstyle='results-even'; }

		route_desc += '<tr class="'+rowstyle+'">';
		
		route_desc += '<td class="result-directions">';
		route_desc += '<img width="18px" src="images/'+getDirectionIcon(response.route_instructions[i][0])+'"/>';
		route_desc += "</td>";		
		
		route_desc += '<td class="result-items">';
		route_desc += '<span class="result-item" onclick="onClickRouteDescription('+response.route_instructions[i][3]+')">';
		route_desc += response.route_instructions[i][0];
		if( i == 0 )
			route_desc += ' ' + OSRM.loc( response.route_instructions[i][6] );		
		if( response.route_instructions[i][1] != "" ) {
			route_desc += ' on ';
			route_desc += '<b>' + response.route_instructions[i][1] + '</b>';
		}
		//route_desc += ' for ';
		route_desc += '</span>';
		route_desc += "</td>";
		
		route_desc += '<td class="result-distance">';
		if( i != response.route_instructions.length-1 )
		route_desc += '<b>'+getDistanceWithUnit(response.route_instructions[i][2])+'</b>';
		route_desc += "</td>";
		
		route_desc += "</tr>";
	}	
		
	route_desc += '</table>';		
	headline = "";
	headline += OSRM.loc("ROUTE_DESCRIPTION")+":<br>";
	headline += '<div style="float:left;width:40%">';
	headline += "<span class='route-summary'>"
		+ OSRM.loc("DISTANCE")+": " + getDistanceWithUnit(response.route_summary.total_distance)
		+ "<br>"
		+ OSRM.loc("DURATION")+": " + secondsToTime(response.route_summary.total_time)
		+ "</span>";		
	headline +=	'</div>';
	headline += '<div style="float:left;text-align:right;width:60%;">'+route_link+'<br>'+gpx_link+'</div>';

	var output = "";
	output += route_desc;

	document.getElementById('information-box-headline').innerHTML = headline;
	document.getElementById('information-box').innerHTML = output;
}
function showRouteDescriptionSimple(response) {
	headline = OSRM.loc("ROUTE_DESCRIPTION")+":<br>";
	headline += "<span class='route-summary'>"
			+ OSRM.loc("DISTANCE")+": " + getDistanceWithUnit(response.route_summary.total_distance)
			+ "<br>"
			+ OSRM.loc("DURATION")+": " + secondsToTime(response.route_summary.total_time)
			+ "</span>";
	headline += '<br><br>';

	document.getElementById('information-box-headline').innerHTML = headline;
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED")+".<p>";	
}
function showNoRouteDescription() {
	headline = OSRM.loc("ROUTE_DESCRIPTION")+":<br>";
	headline += "<span class='route-summary'>"
			+ OSRM.loc("DISTANCE")+": N/A"
			+ "<br>"
			+ OSRM.loc("DURATION")+": N/A"
			+ "</span>";
	headline += '<br><br>';

	document.getElementById('information-box-headline').innerHTML = headline;
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED")+".<p>";	
}


// unnamed streets display
function showRouteNonames(response) {
	// do not display unnamed streets?
	if( document.getElementById('option-highlight-nonames').checked == false) {
		OSRM.G.route.hideUnnamedRoute();
		return;
	}
		
	// mark geometry positions where unnamed/named streets switch
	var named = [];
	for (var i = 0; i < response.route_instructions.length; i++) {
		if( response.route_instructions[i][1] == '' )
			named[ response.route_instructions[i][3] ] = false;		// no street name
		else
			named[ response.route_instructions[i][3] ] = true;		// yes street name
	}

	// aggregate geometry for unnamed streets
	var geometry = decodeRouteGeometry(response.route_geometry, 5);
	var is_named = true;
	var current_positions = [];
	var all_positions = [];
	for( var i=0; i < geometry.length; i++) {
		current_positions.push( new L.LatLng(geometry[i][0], geometry[i][1]) );

		// still named/unnamed?
		if( (named[i] == is_named || named[i] == undefined) && i != geometry.length-1 )
			continue;

		// switch between named/unnamed!
		if(is_named == false)
			all_positions.push( current_positions );
		current_positions = [];
		current_positions.push( new L.LatLng(geometry[i][0], geometry[i][1]) );
		is_named = named[i];
	}
	
	// display unnamed streets
	OSRM.G.route.showUnnamedRoute(all_positions);
}


//-- main function --

// generate server calls to query routes
function getRoute(do_description) {
	
	// if source or target are not set -> hide route
	if( OSRM.G.markers.route.length < 2 ) {
		OSRM.G.route.hideRoute();
		return;
	}
	
	// prepare JSONP call
	var type = null;
	var callback = null;
	var timeout = null;
	
	var source = OSRM.DEFAULTS.HOST_ROUTING_URL;
	source += '?z=' + OSRM.G.map.getZoom() + '&output=json' + '&geomformat=cmp';	
	if(OSRM.G.markers.checksum)
		source += '&checksum=' + OSRM.G.markers.checksum;
	for(var i=0; i<OSRM.G.markers.route.length; i++) {
		source += '&loc='  + OSRM.G.markers.route[i].getLat() + ',' + OSRM.G.markers.route[i].getLng();
		if( OSRM.G.markers.route[i].hint)
			source += '&hint=' + OSRM.G.markers.route[i].hint;
	}
	
	// decide whether it is a dragging call or a normal one
	if (do_description) {
		callback = showRoute;
		timeout = timeoutRoute;
		source +='&instructions=true';
		type = 'route';
	} else {
		callback = showRouteSimple;
		timeout = timeoutRouteSimple;
		source +='&instructions=false';
		type = 'dragging';
	}
	
	// do call
	var called = OSRM.JSONP.call(source, callback, timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, type);

	// TODO: hack to process final drag event, if it was fenced, but we are still dragging
	if(called == false && !do_description) {
		clearTimeout(OSRM.G.pendingTimer);
		OSRM.G.pendingTimer = setTimeout(timeoutDrag,OSRM.DEFAULTS.JSONP_TIMEOUT);
	}
	else {
		clearTimeout(OSRM.G.pendingTimer);
	}
//	// TODO: hack to process final drag event, if it was fenced, but we are still dragging (alternative approach)  
//	if(called == false && !do_description) {
//		OSRM.G.pending = true;
//	} else {
//		clearTimeout(OSRM.G.pendingTimer);
//		OSRM.G.pending = false;
//	}
}
function timeoutDrag() {
	OSRM.G.markers.route[OSRM.G.dragid].hint = null;
	getRoute(OSRM.C.NO_DESCRIPTION);
}


//-- helper functions --

//decode compressed route geometry
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

// update hints of all markers
function updateHints(response) {
	var hint_locations = response.hint_data.locations;
	OSRM.G.markers.checksum = response.hint_data.checksum;
	for(var i=0; i<hint_locations.length; i++)
		OSRM.G.markers.route[i].hint = hint_locations[i];
}

// snap all markers to the received route
function snapRoute() {
	var positions = OSRM.G.route.getPositions();
 	
 	OSRM.G.markers.route[0].setPosition( positions[0] );
 	OSRM.G.markers.route[OSRM.G.markers.route.length-1].setPosition( positions[positions.length-1] );
 	for(var i=0; i<OSRM.G.via_points.length; i++)
		OSRM.G.markers.route[i+1].setPosition( new L.LatLng(OSRM.G.via_points[i][0], OSRM.G.via_points[i][1]) );

 	updateAddress(OSRM.C.SOURCE_LABEL);
 	updateAddress(OSRM.C.TARGET_LABEL);
}

// map driving instructions to icons
// [TODO: better implementation, language-safe]
function getDirectionIcon(name) {
	var directions = {
		"Turn left":"turn-left.png",
		"Turn right":"turn-right.png",
		"U-Turn":"u-turn.png",
		"Head":"continue.png",
		"Continue":"continue.png",
		"Turn slight left":"slight-left.png",
		"Turn slight right":"slight-right.png",
		"Turn sharp left":"sharp-left.png",
		"Turn sharp right":"sharp-right.png",
		"Enter roundabout and leave at first exit":"round-about.png",
		"Enter roundabout and leave at second exit":"round-about.png",
		"Enter roundabout and leave at third exit":"round-about.png",
		"Enter roundabout and leave at fourth exit":"round-about.png",
		"Enter roundabout and leave at fifth exit":"round-about.png",
		"Enter roundabout and leave at sixth exit":"round-about.png",
		"Enter roundabout and leave at seventh exit":"round-about.png",
		"Enter roundabout and leave at eighth exit":"round-about.png",
		"Enter roundabout and leave at nineth exit":"round-about.png",
		"Enter roundabout and leave at tenth exit":"round-about.png",
		"Enter roundabout and leave at one of the too many exit":"round-about.png",
		"You have reached your destination":"target.png"
	};
	
	if( directions[name] )
		return directions[name];
	else
		return "default.png";
}


// -- gui functions --

// click: button "reset"
function resetRouting() {
	document.getElementById('input-source-name').value = "";
	document.getElementById('input-target-name').value = "";
	
	OSRM.G.route.hideAll();
	OSRM.G.markers.removeAll();
	OSRM.G.markers.highlight.hide();
	
	document.getElementById('information-box').innerHTML = "";
	document.getElementById('information-box-headline').innerHTML = "";
	
	OSRM.JSONP.reset();	
}

// click: button "reverse"
function reverseRouting() {
	// invert input boxes
	var tmp = document.getElementById("input-source-name").value;
	document.getElementById("input-source-name").value = document.getElementById("input-target-name").value;
	document.getElementById("input-target-name").value = tmp;
	
	// invert route
	OSRM.G.markers.route.reverse();
	if(OSRM.G.markers.route.length == 1) {
		if(OSRM.G.markers.route[0].label == OSRM.C.TARGET_LABEL) {
			OSRM.G.markers.route[0].label = OSRM.C.SOURCE_LABEL;
			OSRM.G.markers.route[0].marker.setIcon( new L.Icon('images/marker-source.png') );
		} else if(OSRM.G.markers.route[0].label == OSRM.C.SOURCE_LABEL) {
			OSRM.G.markers.route[0].label = OSRM.C.TARGET_LABEL;
			OSRM.G.markers.route[0].marker.setIcon( new L.Icon('images/marker-target.png') );
		}
	} else if(OSRM.G.markers.route.length > 1){
		OSRM.G.markers.route[0].label = OSRM.C.SOURCE_LABEL;
		OSRM.G.markers.route[0].marker.setIcon( new L.Icon('images/marker-source.png') );
		
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].label = OSRM.C.TARGET_LABEL;
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].marker.setIcon( new L.Icon('images/marker-target.png') );		
	}
	
	// recompute route
	if( OSRM.G.route.isShown() ) {
		getRoute(OSRM.C.FULL_DESCRIPTION);
		OSRM.G.markers.highlight.hide();
	} else {
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-headline').innerHTML = "";		
	}
}

// click: button "show"
function showMarker(marker_id) {
	if( OSRM.JSONP.fences["geocoder_source"] || OSRM.JSONP.fences["geocoder_target"] )
		return;
	
	if( marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource() )
		OSRM.G.markers.route[0].centerView();
	else if( marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() )
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].centerView();
}


// changed: any inputbox (is called when return is pressed [after] or focus is lost [before])
function inputChanged(marker_id) {
	if( marker_id == OSRM.C.SOURCE_LABEL)	
		callGeocoder(OSRM.C.SOURCE_LABEL, document.getElementById('input-source-name').value);
	else if( marker_id == OSRM.C.TARGET_LABEL)
		callGeocoder(OSRM.C.TARGET_LABEL, document.getElementById('input-target-name').value);
}
