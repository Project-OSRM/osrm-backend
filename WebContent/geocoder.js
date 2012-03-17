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
OSRM.GEOCODE_POST = 'http://nominatim.openstreetmap.org/search?format=json&bounded=1&viewbox=-27.0,72.0,46.0,36.0';
OSRM.SOURCE_MARKER_LABEL = "source";
OSRM.TARGET_MARKER_LABEL = "target";


// update geo coordinates in input boxes
function updateLocation(marker_id) {
	if (marker_id == OSRM.SOURCE_MARKER_LABEL && my_markers.route[0].dirty_move == true ) {
		document.getElementById("input-source-name").value = my_markers.route[0].getPosition().lat.toFixed(6) + ", " + my_markers.route[0].getPosition().lng.toFixed(6);
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL && my_markers.route[my_markers.route.length-1].dirty_move == true) {
		document.getElementById("input-target-name").value = my_markers.route[my_markers.route.length-1].getPosition().lat.toFixed(6) + ", " + my_markers.route[my_markers.route.length-1].getPosition().lng.toFixed(6);		
	}
}


// process input request and call geocoder if needed
function callGeocoder(marker_id, query) {
	if (marker_id == OSRM.SOURCE_MARKER_LABEL && my_markers.hasSource() && my_markers.route[0].dirty_move == false && my_markers.route[0].dirty_type == false)
		return;
	if (marker_id == OSRM.TARGET_MARKER_LABEL && my_markers.hasTarget() && my_markers.route[my_markers.route.length-1].dirty_move == false && my_markers.route[my_markers.route.length-1].dirty_type == false)
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
	if (marker_id == OSRM.SOURCE_MARKER_LABEL) {
		var src= OSRM.GEOCODE_POST + "&q=" + query;
		OSRM.JSONP.call( src, showGeocoderResults_Source, showGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "geocoder_source" );
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL) {
		var src = OSRM.GEOCODE_POST + "&q=" + query; 
		OSRM.JSONP.call( src, showGeocoderResults_Target, showGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "geocoder_target" );		
	}
}


// helper function for clicks on geocoder search results
function onclickGeocoderResult(marker_id, lat, lon, do_reverse_geocode, zoom ) {
	var index;
	if( marker_id == OSRM.SOURCE_MARKER_LABEL )
		index = my_markers.setSource( new L.LatLng(lat, lon) );
	else if( marker_id == OSRM.TARGET_MARKER_LABEL )
		index = my_markers.setTarget( new L.LatLng(lat, lon) );
	else
		index = -1;													// via nodes not yet implemented
	
	if( do_reverse_geocode == true )
		updateReverseGeocoder(marker_id);
	if( zoom == undefined )
		zoom = true;
		
	my_markers.route[index].show();
	if( !my_markers.route[index].dirty_move || my_markers.route[index].dirty_type )
		my_markers.route[index].centerView(zoom);	
	getRoute(OSRM.FULL_DESCRIPTION);
	
	my_markers.route[index].dirty_move = false;
	my_markers.route[index].dirty_type = false;
}

// process JSONP response of geocoder
// (with wrapper functions for source/target jsonp)
function showGeocoderResults_Source(response) {	showGeocoderResults(OSRM.SOURCE_MARKER_LABEL, response); }
function showGeocoderResults_Target(response) {	showGeocoderResults(OSRM.TARGET_MARKER_LABEL, response); }
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
	if(marker_id == OSRM.SOURCE_MARKER_LABEL)
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND_SOURCE")+".<p>";
	else if(marker_id == OSRM.TARGET_MARKER_LABEL)
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND_TARGET")+".<p>";
	else
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND")+".<p>";
}
function showGeocoderResults_Timeout() {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";	
}


// - [upcoming feature: reverse geocoding (untested) ] -
OSRM.REVERSE_GEOCODE_POST = 'http://nominatim.openstreetmap.org/reverse?format=json&bounded=1&viewbox=-27.0,72.0,46.0,36.0';

//update reverse geocoder informatiopn in input boxes
function updateReverseGeocoder(marker_id) {
	if (marker_id == OSRM.SOURCE_MARKER_LABEL && my_markers.hasSource()) { //&& my_markers.route[0].dirty == true ) {
		//document.getElementById("input-source-name").value = my_markers.route[0].getPosition().lat.toFixed(6) + ", " + my_markers.route[0].getPosition().lng.toFixed(6);
		callReverseGeocoder("source", my_markers.route[0].getPosition().lat, my_markers.route[0].getPosition().lng);
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL && my_markers.hasTarget()) { //&& my_markers.route[my_markers.route.length-1].dirty == true) {
		//document.getElementById("input-target-name").value = my_markers.route[my_markers.route.length-1].getPosition().lat.toFixed(6) + ", " + my_markers.route[my_markers.route.length-1].getPosition().lng.toFixed(6);
		callReverseGeocoder("target", my_markers.route[my_markers.route.length-1].getPosition().lat, my_markers.route[my_markers.route.length-1].getPosition().lng);
	}
}

//prepare request and call reverse geocoder
function callReverseGeocoder(marker_id, lat, lon) {
	//build request
	if (marker_id == OSRM.SOURCE_MARKER_LABEL) {
		var src= OSRM.REVERSE_GEOCODE_POST + "&lat=" + lat + "&lon=" + lon;
		OSRM.JSONP.call( src, showReverseGeocoderResults_Source, showReverseGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "reverse_geocoder_source" );
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL) {
		var src = OSRM.REVERSE_GEOCODE_POST + "&lat=" + lat + "&lon=" + lon;
		OSRM.JSONP.call( src, showReverseGeocoderResults_Target, showReverseGeocoderResults_Timeout, OSRM.DEFAULTS.JSONP_TIMEOUT, "reverse_geocoder_target" );
	}
}
//processing JSONP response of reverse geocoder
//(with wrapper functions for source/target jsonp)
function showReverseGeocoderResults_Timeout() {}
function showReverseGeocoderResults_Source(response) {	showReverseGeocoderResults(OSRM.SOURCE_MARKER_LABEL, response); }
function showReverseGeocoderResults_Target(response) {	showReverseGeocoderResults(OSRM.TARGET_MARKER_LABEL, response); }
function showReverseGeocoderResults(marker_id, response) {
	//OSRM.debug.log("[inner] reverse geocoder");
	if(response){
		if(response.address == undefined)
			return;

		var address = "";
		if( response.address.road)
			address += response.address.road;	
		if( response.address.city ) {
			if( address != "" )
				address += ", ";
			address += response.address.city;
		} else if( response.address.village ) {
			if( address != "" )
				address += ", ";
			address += response.address.village;
		}
		if( address == "" && response.address.country )
			address += response.address.country;
		if( address == "" )
			return;
		
		if(marker_id == OSRM.SOURCE_MARKER_LABEL) {
			document.getElementById("input-source-name").value = address;
			my_markers.route[0].dirty_move = false;
			my_markers.route[0].dirty_type = false;
		} else if(marker_id == OSRM.TARGET_MARKER_LABEL) {
			document.getElementById("input-target-name").value = address;
			my_markers.route[my_markers.route.length-1].dirty_move = false;
			my_markers.route[my_markers.route.length-1].dirty_type = false;
		}
		
	}
}
