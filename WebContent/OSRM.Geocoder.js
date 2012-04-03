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
	
	//geo coordinates given -> directly draw results
	if(query.match(/^\s*[-+]?[0-9]*\.?[0-9]+\s*[,;]\s*[-+]?[0-9]*\.?[0-9]+\s*$/)){
		var coord = query.split(/[,;]/);
		OSRM.Geocoder._onclickResult(marker_id, coord[0], coord[1]);
		OSRM.Geocoder.updateAddress( marker_id );
		return;
	}
	
	//build request for geocoder
	var call = OSRM.DEFAULTS.HOST_GEOCODER_URL + "?format=json" + OSRM.DEFAULTS.GEOCODER_BOUNDS + "&q=" + query;
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
	
	// show possible results for input
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
			html += '<div class="result-item" onclick="OSRM.Geocoder._onclickResult(\''+parameters.marker_id+'\', '+result.lat+', '+result.lon+');">'+result.display_name+'</div>';
		}
		html += "</td></tr>";
	}
	html += '</table>';
		
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	document.getElementById('information-box').innerHTML = html;

	OSRM.Geocoder._onclickResult(parameters.marker_id, response[0].lat, response[0].lon);
},
_showResults_Empty: function(parameters) {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	if(parameters.marker_id == OSRM.C.SOURCE_LABEL)
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND_SOURCE")+": "+parameters.query +".<p>";
	else if(parameters.marker_id == OSRM.C.TARGET_LABEL)
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND_TARGET")+": "+parameters.query +".<p>";
	else
		document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND")+": "+parameters.query +".<p>";
},
_showResults_Timeout: function() {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";	
},


// [reverse geocoding]

//update geo coordinates in input boxes
updateLocation: function(marker_id) {
	if (marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource()) {
		document.getElementById("input-source-name").value = OSRM.G.markers.route[0].getLat().toFixed(6) + ", " + OSRM.G.markers.route[0].getLng().toFixed(6);
	} else if (marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget()) {
		document.getElementById("input-target-name").value = OSRM.G.markers.route[OSRM.G.markers.route.length-1].getLat().toFixed(6) + ", " + OSRM.G.markers.route[OSRM.G.markers.route.length-1].getLng().toFixed(6);		
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
	
	var call = OSRM.DEFAULTS.HOST_REVERSE_GEOCODER_URL + "?format=json" + "&lat=" + lat + "&lon=" + lng;
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
		document.getElementById("input-source-name").value = address;
	else if(parameters.marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() )
		document.getElementById("input-target-name").value = address;
},
_showReverseResults_Timeout: function(response, parameters) {
	if(!parameters.do_fallback)
		return;
		
	updateLocation(parameters.marker_id);
}

};