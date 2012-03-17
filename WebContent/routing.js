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
var my_route = undefined;
var my_markers = undefined;

OSRM.NO_DESCRIPTION = 0;
OSRM.FULL_DESCRIPTION = 1;
OSRM.dragging = false;
OSRM.pending = false;
OSRM.pendingTimer = undefined;


// init routing data structures
function initRouting() {
	my_route = new OSRM.Route();
	my_markers = new OSRM.Markers();
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
	my_route.hideUnnamedRoute();
	showNoRouteDescription();
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";	
}
function showRouteSimple(response) {
 	if(!response)
 		return;
 	
 	if (OSRM.JSONP.fences.route)			// prevent simple routing when real routing is done!
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
//	if(OSRM.pending) {
//		clearTimeout(OSRM.pendingTimer);
//		OSRM.pendingTimer = setTimeout(timeoutDrag,100);		
//	}
}
function showRoute(response) {
	if(!response)
		return;
	
	if(response.status == 207) {
		showNoRouteGeometry();
		my_route.hideUnnamedRoute();
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
	for(var i=0; i<my_markers.route.length;i++)
			positions.push( my_markers.route[i].getPosition() );

	my_route.showRoute(positions, OSRM.Route.NOROUTE);
}
function showRouteGeometry(response) {
	via_points = response.via_points.slice(0);

	var geometry = decodeRouteGeometry(response.route_geometry, 5);

	var points = [];
	for( var i=0; i < geometry.length; i++) {
		points.push( new L.LatLng(geometry[i][0], geometry[i][1]) );
	}
	my_route.showRoute(points, OSRM.Route.ROUTE);
}


// route description display (and helper functions)
function onClickRouteDescription(geometry_index) {
	var positions = my_route.getPositions();

	my_markers.highlight.setPosition( positions[geometry_index] );
	my_markers.highlight.show();
	my_markers.highlight.centerView();	
}
function onClickCreateShortcut(src){
	OSRM.JSONP.call(OSRM.DEFAULTS.HOST_SHORTENER_URL+src+'&jsonp=showRouteLink', showRouteLink, showRouteLink_TimeOut, 2000, 'shortener');
}
function showRouteLink(response){
	document.getElementById('route-prelink').innerHTML = '[<a id="gpx-link" class = "text-selectable" href="' +response.ShortURL+ '">'+response.ShortURL+'</a>]';
}
function showRouteLink_TimeOut(){
	document.getElementById('route-prelink').innerHTML = '['+OSRM.loc("LINK_TO_ROUTE_TIMEOUT")+']';
}
function showRouteDescription(response) {
	// compute query string
	var query_string = '?z='+ map.getZoom();
	query_string += '&center=' + map.center.getLat() + ',' + map.center.getLng();
	for(var i=0; i<my_markers.route.length; i++)
		query_string += '&loc=' + my_markers.route[i].getLat() + ',' + my_markers.route[i].getLng(); 
 						
	// create link to the route
	var route_link ='<span class="route-summary" id="route-prelink">[<a id="gpx-link" href="#" onclick="onClickCreateShortcut(\'' + OSRM.DEFAULTS.WEBSITE_URL + query_string + '\')">'+OSRM.loc("GET_LINK")+'</a>]</span>';

	// create GPX link
	var gpx_link = '<span class="route-summary">[<a id="gpx-link" onClick="javascript: document.location.href=\'' + OSRM.DEFAULTS.HOST_ROUTING_URL + query_string + '&output=gpx\';">'+OSRM.loc("GPX_FILE")+'</a>]</span>';
		
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
		my_route.hideUnnamedRoute();
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
	my_route.showUnnamedRoute(all_positions);
}


//-- main function --

// generate server calls to query routes
function getRoute(do_description) {
	
	// if source or target are not set -> hide route
	if( my_markers.route.length < 2 ) {
		my_route.hideRoute();
		return;
	}
	
	// prepare JSONP call
	var type = undefined;
	var callback = undefined;
	var timeout = undefined;
	
	var source = OSRM.DEFAULTS.HOST_ROUTING_URL;
	source += '?z=' + map.getZoom() + '&output=json' + '&geomformat=cmp';	
	if(my_markers.checksum)
		source += '&checksum=' + my_markers.checksum;
	for(var i=0; i<my_markers.route.length; i++) {
		source += '&loc='  + my_markers.route[i].getLat() + ',' + my_markers.route[i].getLng();
		if( my_markers.route[i].hint)
			source += '&hint=' + my_markers.route[i].hint;
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
		clearTimeout(OSRM.pendingTimer);
		OSRM.pendingTimer = setTimeout(timeoutDrag,OSRM.DEFAULTS.JSONP_TIMEOUT);
	}
	else {
		clearTimeout(OSRM.pendingTimer);
	}
//	// TODO: hack to process final drag event, if it was fenced, but we are still dragging (alternative approach)  
//	if(called == false && !do_description) {
//		OSRM.pending = true;
//	} else {
//		clearTimeout(OSRM.pendingTimer);
//		OSRM.pending = false;
//	}
}
function timeoutDrag() {
	my_markers.route[OSRM.dragid].hint = undefined;
	getRoute(OSRM.NO_DESCRIPTION);
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
	my_markers.checksum = response.hint_data.checksum;
	for(var i=0; i<hint_locations.length; i++)
		my_markers.route[i].hint = hint_locations[i];
}

// snap all markers to the received route
function snapRoute() {
	var positions = my_route.getPositions();
 	
 	my_markers.route[0].setPosition( positions[0] );
 	my_markers.route[my_markers.route.length-1].setPosition( positions[positions.length-1] );
 	for(var i=0; i<via_points.length; i++)
		my_markers.route[i+1].setPosition( new L.LatLng(via_points[i][0], via_points[i][1]) );

// 	updateLocation( "source" );
// 	updateLocation( "target" );
	
	//if(OSRM.dragid == 0 && my_markers.hasSource()==true)
		updateReverseGeocoder("source");
	//else if(OSRM.dragid == my_markers.route.length-1 && my_markers.hasTarget()==true)
		updateReverseGeocoder("target"); 	
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
		"Enter roundabout and leave at forth exit":"round-about.png",
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
	
	my_route.hideAll();
	my_markers.removeAll();
	my_markers.highlight.hide();
	
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
	my_markers.route.reverse();
	if(my_markers.route.length == 1) {
		if(my_markers.route[0].label == OSRM.TARGET_MARKER_LABEL) {
			my_markers.route[0].label = OSRM.SOURCE_MARKER_LABEL;
			my_markers.route[0].marker.setIcon( new L.Icon('images/marker-source.png') );
		} else if(my_markers.route[0].label == OSRM.SOURCE_MARKER_LABEL) {
			my_markers.route[0].label = OSRM.TARGET_MARKER_LABEL;
			my_markers.route[0].marker.setIcon( new L.Icon('images/marker-target.png') );
		}
	} else if(my_markers.route.length > 1){
		my_markers.route[0].label = OSRM.SOURCE_MARKER_LABEL;
		my_markers.route[0].marker.setIcon( new L.Icon('images/marker-source.png') );
		
		my_markers.route[my_markers.route.length-1].label = OSRM.TARGET_MARKER_LABEL;
		my_markers.route[my_markers.route.length-1].marker.setIcon( new L.Icon('images/marker-target.png') );		
	}
	
	// recompute route
	if( my_route.isShown() ) {
		getRoute(OSRM.FULL_DESCRIPTION);
		my_markers.highlight.hide();
	} else {
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-headline').innerHTML = "";		
	}
}

// click: button "show"
function centerMarker(marker_id) {
	if( marker_id == OSRM.SOURCE_MARKER_LABEL && my_markers.route[0] && my_markers.route[0].label == OSRM.SOURCE_MARKER_LABEL && !my_markers.route[0].dirty_type ) {
		my_markers.route[0].centerView();
	} else if( marker_id == OSRM.TARGET_MARKER_LABEL && my_markers.route[my_markers.route.length-1] && my_markers.route[my_markers.route.length-1].label == OSRM.TARGET_MARKER_LABEL && !my_markers.route[my_markers.route.length-1].dirty_type) {
		my_markers.route[my_markers.route.length-1].centerView();
	}
}
