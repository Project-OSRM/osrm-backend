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

// OSRM geocoding routines
// [geocoder query, management and display of geocoder results]

// some constants
OSRM.CONSTANTS.SOURCE_LABEL = "source";
OSRM.CONSTANTS.TARGET_LABEL = "target";
OSRM.CONSTANTS.VIA_LABEL = "via";
OSRM.CONSTANTS.DO_FALLBACK_TO_LAT_LNG = true;


OSRM.Geocoder = {

//[normal geocoding]

// process input request and call geocoder if needed
call: function(marker_id, query) {
	if(query=="")
		return;
	
	// geo coordinates given -> directly draw results
	if(query.match(/^\s*[-+]?[0-9]*\.?[0-9]+\s*[,;]\s*[-+]?[0-9]*\.?[0-9]+\s*$/)){
		var coord = query.split(/[,;]/);
		OSRM.Geocoder._onclickResult(marker_id, coord[0], coord[1]);
		OSRM.Geocoder.updateAddress( marker_id );
		return;
	}
	
	// build basic request for geocoder
	var call = OSRM.DEFAULTS.HOST_GEOCODER_URL + "?format=json&json_callback=%jsonp" + OSRM.DEFAULTS.GEOCODER_BOUNDS + "&accept-language="+OSRM.Localization.current_language+"&limit=30&q=" + query;
	// prioritize results in currently shown mapview
	var bounds = OSRM.G.map.getBounds();
	call += "&viewbox=" + bounds._southWest.lat + "," + bounds._northEast.lng + "," + bounds._northEast.lat + "," + bounds._southWest.lng;
	OSRM.JSONP.call( call, OSRM.Geocoder._showResults, OSRM.Geocoder._showResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "geocoder_"+marker_id, {marker_id:marker_id,query:query} );
},


// helper function for clicks on geocoder search results
_onclickResult: function(marker_id, lat, lon) {
	var index;
	if( marker_id == OSRM.C.SOURCE_LABEL )
		index = OSRM.G.markers.setSource( new L.LatLng(lat, lon) );
	else if( marker_id == OSRM.C.TARGET_LABEL )
		index = OSRM.G.markers.setTarget( new L.LatLng(lat, lon) );
	else
		return;
	
	OSRM.G.markers.route[index].show();
	OSRM.G.markers.route[index].centerView();	
	if( OSRM.G.markers.route.length > 1 )
		OSRM.Routing.getRoute();
},


// process geocoder response
_showResults: function(response, parameters) {
	if(!response){
		OSRM.Geocoder._showResults_Empty(parameters);
		return;
	}
	if(response.length == 0) {
		OSRM.Geocoder._showResults_Empty(parameters);
		return;
	}	
	
	// filter/sort inputs
	var filtered_response_temp = [];									// filter results
	for(var i=0, iEnd=response.length; i < iEnd; i++){
		var result = response[i];
		if( OSRM.Geocoder._filterResult( result ) )
			continue;
		filtered_response_temp.push( result );
	}
	if(filtered_response_temp.length == 0) {							// stop if no results remain
		OSRM.Geocoder._showResults_Empty(parameters);
		return;
	}
	filtered_response_temp.sort( OSRM.Geocoder._compareResults );		// rank results
	filtered_response_temp.sort( OSRM.Geocoder._compareLocations );		// remove duplicate locations (stable sort -> retain highest ranked)
	var filtered_response = [];
	filtered_response.push( filtered_response_temp[0] );
	for(var i=1, iEnd = filtered_response_temp.length; i<iEnd; i++) {
		var prev_result = filtered_response_temp[i-1];
		var result = filtered_response_temp[i];
		if( result.lat != prev_result.lat || result.lon != prev_result.lon ) {
			filtered_response.push( result );
		}
	}
	filtered_response.sort( OSRM.Geocoder._compareResults );			// rank results again
	
	// show first result
	OSRM.Geocoder._onclickResult(parameters.marker_id, filtered_response[0].lat, filtered_response[0].lon);
	if( OSRM.G.markers.route.length > 1 )		// if a route is displayed, we don't need to show other possible geocoding results
		return;	
	
	// show possible results for input
	var html = "";
	html += '<table class="results medium-font">';	
	for(var i=0; i < filtered_response.length; i++){
		var result = filtered_response[i];
		
		//odd or even ?
		var rowstyle='results-body-odd';
		if(i%2==0) { rowstyle='results-body-even'; }
 
		html += '<tr class="'+rowstyle+'">';
		if(!result.icon)
			result.icon = "http://nominatim.openstreetmap.org/images/mapicons/poi_point_of_interest.glow.12.png";
		html += '<td class="results-body-counter"><img src="'+ result.icon + '" alt=""/></td>';
		html += '<td class="results-body-items">';

		if(result.display_name){
			html += '<div class="results-body-item" onclick="OSRM.Geocoder._onclickResult(\''+parameters.marker_id+'\', '+result.lat+', '+result.lon+');">'+result.display_name;
			// debug output to show osm_type, class, type			
			// html += '<br/><span class="results-body-item-remark small-font">[osm_type: ' + result.osm_type + ', class: ' + result.class + ', type: ' + result.type + ']</span>';
			html += '</div>';
		}
		html += "</td></tr>";
	}
	html += '</table>';
		
	document.getElementById('information-box-header').innerHTML = 
		"<div class='header-title'>"+OSRM.loc("SEARCH_RESULTS")+"</div>" +
		"<div class='header-content'>("+OSRM.loc("FOUND_X_RESULTS").replace(/%i/,filtered_response.length)+")</div>";
		"<div class='header-content'>(found "+filtered_response.length+" results)"+"</div>";
	document.getElementById('information-box').innerHTML = html;
},
_showResults_Empty: function(parameters) {
	document.getElementById('information-box-header').innerHTML =
		"<div class='header-title'>"+OSRM.loc("SEARCH_RESULTS")+"</div>" +
		"<div class='header-content'>("+OSRM.loc("FOUND_X_RESULTS").replace(/%i/,0)+")</div>";		
	if(parameters.marker_id == OSRM.C.SOURCE_LABEL)
		document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+OSRM.loc("NO_RESULTS_FOUND_SOURCE")+": "+parameters.query +"</div>";
	else if(parameters.marker_id == OSRM.C.TARGET_LABEL)
		document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+OSRM.loc("NO_RESULTS_FOUND_TARGET")+": "+parameters.query +"</div>";
	else
		document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+OSRM.loc("NO_RESULTS_FOUND")+": "+parameters.query +"</div>";
},
_showResults_Timeout: function() {
	document.getElementById('information-box-header').innerHTML =
		"<div class='header-title'>"+OSRM.loc("SEARCH_RESULTS")+"</div>" +
		"<div class='header-content'>("+OSRM.loc("FOUND_X_RESULTS").replace(/%i/,0)+")</div>";		
	document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+OSRM.loc("TIMED_OUT")+"</div>";	
},


// filter search results [true: result will not be displayed]
_filterResult: function(result) {
//	if( result.osm_type == "relation")
//		return true;
	if( result.type == "aerial_views")
		return true;	
	return false;
},


// comparator for sorting results [higher weight: result will appear first]
_compare_class_weights: {
	boundary: 9000,	
	place: 8000,
	highway: 7000,
}, 
_compare_type_weights: {
	country: 13,
	state: 12,
	county: 11,
	city: 10, 
	town: 9,
	village: 8,
	hamlet: 7,
	suburb: 6,
	locality: 5,
	farm: 4
},
_compareResults: function(lhs, rhs) {
	var class_values = OSRM.Geocoder._compare_class_weights;
	var type_values = OSRM.Geocoder._compare_type_weights;
	
	var lhs_value = (-class_values[ lhs["class"] ] || 0) + (-type_values[ lhs.type ] || 0);
	var rhs_value = (-class_values[ rhs["class"] ] || 0) + (-type_values[ rhs.type ] || 0);

	return (lhs_value - rhs_value);
},


// comparator for sorting objects according to their locations
_compareLocations: function(lhs, rhs) {
	if( lhs.lat != rhs.lat )
		return lhs.lat < rhs.lat;
	else
		return lhs.lon < rhs.lon;
},


// [reverse geocoding]

//update geo coordinates in input boxes
updateLocation: function(marker_id) {
	if (marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource()) {
		document.getElementById("gui-input-source").value = OSRM.G.markers.route[0].getLat().toFixed(6) + ", " + OSRM.G.markers.route[0].getLng().toFixed(6);
	} else if (marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget()) {
		document.getElementById("gui-input-target").value = OSRM.G.markers.route[OSRM.G.markers.route.length-1].getLat().toFixed(6) + ", " + OSRM.G.markers.route[OSRM.G.markers.route.length-1].getLng().toFixed(6);		
	}
},


// update address in input boxes
updateAddress: function(marker_id, do_fallback_to_lat_lng) {
	// build request for reverse geocoder
	var lat = null;
	var lng = null;
	
	if(marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource()) {
		lat = OSRM.G.markers.route[0].getLat();
		lng = OSRM.G.markers.route[0].getLng();
	} else if(marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() ) {
		lat = OSRM.G.markers.route[OSRM.G.markers.route.length-1].getLat();
		lng = OSRM.G.markers.route[OSRM.G.markers.route.length-1].getLng();
	} else
		return;
	
	var call = OSRM.DEFAULTS.HOST_REVERSE_GEOCODER_URL + "?format=json&json_callback=%jsonp" + "&accept-language="+OSRM.Localization.current_language + "&lat=" + lat.toFixed(6) + "&lon=" + lng.toFixed(6);
	OSRM.JSONP.call( call, OSRM.Geocoder._showReverseResults, OSRM.Geocoder._showReverseResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "reverse_geocoder_"+marker_id, {marker_id:marker_id, do_fallback: do_fallback_to_lat_lng} );
},


// processing JSONP response of reverse geocoder
_showReverseResults: function(response, parameters) {
 	if(!response) {
 		OSRM.Geocoder._showReverseResults_Timeout(response, parameters);
		return;
	}
 	
	if(response.address == undefined) {
		OSRM.Geocoder._showReverseResults_Timeout(response, parameters);
		return;
	}

	// build reverse geocoding address
	var used_address_data = 0;
	var address = "";
	if( response.address.road) {
		address += response.address.road;
		used_address_data++;
	}
	if( response.address.city ) {
		if( used_address_data > 0 )
			address += ", ";
		address += response.address.city;
		used_address_data++;
	} else if( response.address.village ) {
		if( used_address_data > 0 )
			address += ", ";
		address += response.address.village;
		used_address_data++;
	}
	if( used_address_data < 2 && response.address.country ) {
		if( used_address_data > 0 )
			address += ", ";
		address += response.address.country;
		used_address_data++;
	}
	if( used_address_data == 0 ) {
		OSRM.Geocoder._showReverseResults_Timeout(response, parameters);
		return;
	}
		
	// add result to DOM
	if(parameters.marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource() )
		document.getElementById("gui-input-source").value = address;
	else if(parameters.marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() )
		document.getElementById("gui-input-target").value = address;
},
_showReverseResults_Timeout: function(response, parameters) {
	if(!parameters.do_fallback)
		return;
		
	OSRM.Geocoder.updateLocation(parameters.marker_id);
}

};
