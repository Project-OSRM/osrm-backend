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
// [TODO: better separation of GUI and geocoding routines, reverse geocoding]

// some constants
OSRM.CONSTANTS.SOURCE_LABEL = "source";
OSRM.CONSTANTS.TARGET_LABEL = "target";
OSRM.CONSTANTS.VIA_LABEL = "via";


// update geo coordinates in input boxes
function updateLocation(marker_id) {
	if (marker_id == OSRM.C.SOURCE_LABEL) {
		document.getElementById("input-source-name").value = OSRM.G.markers.route[0].getPosition().lat.toFixed(6) + ", " + OSRM.G.markers.route[0].getPosition().lng.toFixed(6);
	} else if (marker_id == OSRM.C.TARGET_LABEL) {
		document.getElementById("input-target-name").value = OSRM.G.markers.route[OSRM.G.markers.route.length-1].getPosition().lat.toFixed(6) + ", " + OSRM.G.markers.route[OSRM.G.markers.route.length-1].getPosition().lng.toFixed(6);		
	}
}


// process input request and call geocoder if needed
function callGeocoder(marker_id, query) {
	if (marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource() && OSRM.G.markers.route[0].dirty_move == false && OSRM.G.markers.route[0].dirty_type == false)
		return;
	if (marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() && OSRM.G.markers.route[OSRM.G.markers.route.length-1].dirty_move == false && OSRM.G.markers.route[OSRM.G.markers.route.length-1].dirty_type == false)
		return;
	if(query=="")
		return;
	
	//geo coordinates given -> go directly to drawing results
	if(query.match(/^\s*[-+]?[0-9]*\.?[0-9]+\s*[,;]\s*[-+]?[0-9]*\.?[0-9]+\s*$/)){
		var coord = query.split(/[,;]/);
		onclickGeocoderResult(marker_id, coord[0], coord[1], true);
		return;
	}
	
	//build request 
	if (marker_id == OSRM.C.SOURCE_LABEL) {
		var call = OSRM.DEFAULTS.HOST_GEOCODER_URL + "?format=json" + OSRM.DEFAULTS.GEOCODER_BOUNDS + "&q=" + query;
		OSRM.JSONP.call( call, showGeocoderResults_Source, showGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "geocoder_source" );
	} else if (marker_id == OSRM.C.TARGET_LABEL) {
		var call = OSRM.DEFAULTS.HOST_GEOCODER_URL + "?format=json" + OSRM.DEFAULTS.GEOCODER_BOUNDS + "&q=" + query;
		OSRM.JSONP.call( call, showGeocoderResults_Target, showGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "geocoder_target" );		
	}
}


// helper function for clicks on geocoder search results
function onclickGeocoderResult(marker_id, lat, lon, do_reverse_geocode, do_zoom ) {
	var index;
	if( marker_id == OSRM.C.SOURCE_LABEL )
		index = OSRM.G.markers.setSource( new L.LatLng(lat, lon) );
	else if( marker_id == OSRM.C.TARGET_LABEL )
		index = OSRM.G.markers.setTarget( new L.LatLng(lat, lon) );
	else
		index = -1;													// via nodes not yet implemented
	
	if( do_reverse_geocode == true )
		updateReverseGeocoder(marker_id);
	var zoom = undefined;
	if( do_zoom == false )
		zoom = OSRM.G.map.getZoom();
		
	OSRM.G.markers.route[index].show();
	if( !OSRM.G.markers.route[index].dirty_move || OSRM.G.markers.route[index].dirty_type )
		OSRM.G.markers.route[index].centerView(zoom);	
	getRoute(OSRM.C.FULL_DESCRIPTION);
	
	OSRM.G.markers.route[index].dirty_move = false;
	OSRM.G.markers.route[index].dirty_type = false;
}

// process JSONP response of geocoder
// (with wrapper functions for source/target jsonp)
function showGeocoderResults_Source(response) {	showGeocoderResults(OSRM.C.SOURCE_LABEL, response); }
function showGeocoderResults_Target(response) {	showGeocoderResults(OSRM.C.TARGET_LABEL, response); }
function showGeocoderResults(marker_id, response) {
	if(response){
		if(response.length == 0) {
			showGeocoderResults_Empty(marker_id);
			return;
		}
		
		var html = "";
		html += '<table class="results-table">';	
		for(var i=0; i < response.length; i++){
			var result = response[i];

			//odd or even ?
			var rowstyle='results-odd';
			if(i%2==0) { rowstyle='results-even'; }
 
			html += '<tr class="'+rowstyle+'">';
			html += '<td class="result-counter"><span">'+(i+1)+'.</span></td>';
			html += '<td class="result-items">';

			if(result.display_name){
				html += '<div class="result-item" onclick="onclickGeocoderResult(\''+marker_id+'\', '+result.lat+', '+result.lon+');">'+result.display_name+'</div>';
			}
			html += "</td></tr>";
		}
		html += '</table>';
		
		document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
		document.getElementById('information-box').innerHTML = html;

		onclickGeocoderResult(marker_id, response[0].lat, response[0].lon);
	}
}
function showGeocoderResults_Empty(marker_id) {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	if(marker_id == OSRM.C.SOURCE_LABEL)
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND_SOURCE")+".<p>";
	else if(marker_id == OSRM.C.TARGET_LABEL)
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND_TARGET")+".<p>";
	else
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND")+".<p>";
}
function showGeocoderResults_Timeout() {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";	
}


// - [upcoming feature: reverse geocoding (untested) ] -

//update reverse geocoder informatiopn in input boxes
function updateReverseGeocoder(marker_id) {
	if (marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource()) { //&& OSRM.G.markers.route[0].dirty == true ) {
		//document.getElementById("input-source-name").value = OSRM.G.markers.route[0].getPosition().lat.toFixed(6) + ", " + OSRM.G.markers.route[0].getPosition().lng.toFixed(6);
		callReverseGeocoder(OSRM.C.SOURCE_LABEL, OSRM.G.markers.route[0].getPosition().lat, OSRM.G.markers.route[0].getPosition().lng);
	} else if (marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget()) { //&& OSRM.G.markers.route[OSRM.G.markers.route.length-1].dirty == true) {
		//document.getElementById("input-target-name").value = OSRM.G.markers.route[OSRM.G.markers.route.length-1].getPosition().lat.toFixed(6) + ", " + OSRM.G.markers.route[OSRM.G.markers.route.length-1].getPosition().lng.toFixed(6);
		callReverseGeocoder(OSRM.C.TARGET_LABEL, OSRM.G.markers.route[OSRM.G.markers.route.length-1].getPosition().lat, OSRM.G.markers.route[OSRM.G.markers.route.length-1].getPosition().lng);
	}
}

//prepare request and call reverse geocoder
function callReverseGeocoder(marker_id, lat, lon) {
	//build request
	if (marker_id == OSRM.C.SOURCE_LABEL) {
		var call = OSRM.DEFAULTS.HOST_REVERSE_GEOCODER_URL + "?format=json" + "&lat=" + lat + "&lon=" + lon;
		OSRM.JSONP.call( call, showReverseGeocoderResults_Source, showReverseGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "reverse_geocoder_source" );
	} else if (marker_id == OSRM.C.TARGET_LABEL) {
		var call = OSRM.DEFAULTS.HOST_REVERSE_GEOCODER_URL + "?format=json" + "&lat=" + lat + "&lon=" + lon;
		OSRM.JSONP.call( call, showReverseGeocoderResults_Target, showReverseGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "reverse_geocoder_target" );
	}
}
//processing JSONP response of reverse geocoder
//(with wrapper functions for source/target jsonp)
function showReverseGeocoderResults_Timeout() {}
function showReverseGeocoderResults_Source(response) {	showReverseGeocoderResults(OSRM.C.SOURCE_LABEL, response); }
function showReverseGeocoderResults_Target(response) {	showReverseGeocoderResults(OSRM.C.TARGET_LABEL, response); }
function showReverseGeocoderResults(marker_id, response) {
	if(response){
		if(response.address == undefined)
			return;

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
		if( used_address_data == 0 )
			return;
		
		// add result to DOM
		if(marker_id == OSRM.C.SOURCE_LABEL) {
			document.getElementById("input-source-name").value = address;
			OSRM.G.markers.route[0].dirty_move = false;
			OSRM.G.markers.route[0].dirty_type = false;
		} else if(marker_id == OSRM.C.TARGET_LABEL) {
			document.getElementById("input-target-name").value = address;
			OSRM.G.markers.route[OSRM.G.markers.route.length-1].dirty_move = false;
			OSRM.G.markers.route[OSRM.G.markers.route.length-1].dirty_type = false;
		}
		
	}
}
