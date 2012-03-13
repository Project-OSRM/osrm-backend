// some constants
OSRM.GEOCODE_POST = 'http://nominatim.openstreetmap.org/search?format=json';
OSRM.REVERSE_GEOCODE_POST = 'http://nominatim.openstreetmap.org/reverse?format=json';
OSRM.SOURCE_MARKER_LABEL = "source";
OSRM.TARGET_MARKER_LABEL = "target";


// update locations in input boxes
function updateLocation(marker_id) {
	if (marker_id == OSRM.SOURCE_MARKER_LABEL) {
		document.getElementById("input-source-name").value = my_markers.route[0].getPosition().lat.toFixed(6) + ", " + my_markers.route[0].getPosition().lng.toFixed(6);		
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL) {
		document.getElementById("input-target-name").value = my_markers.route[my_markers.route.length-1].getPosition().lat.toFixed(6) + ", " + my_markers.route[my_markers.route.length-1].getPosition().lng.toFixed(6);		
	}
}
function updateLocations() {
	if( my_markers.route[0] && my_markers.route[0].label == OSRM.SOURCE_MARKER_LABEL) {
		document.getElementById("input-source-name").value = my_markers.route[0].getPosition().lat.toFixed(6) + ", " + my_markers.route[0].getPosition().lng.toFixed(6);
		callReverseGeocoder("source", my_markers.route[0].getPosition().lat, my_markers.route[0].getPosition().lng);
		//OSRM.debug.log("[call1] reverse geocoder");
	}
	
	if( my_markers.route[my_markers.route.length-1] && my_markers.route[ my_markers.route.length-1 ].label == OSRM.TARGET_MARKER_LABEL) {
		document.getElementById("input-target-name").value = my_markers.route[my_markers.route.length-1].getPosition().lat.toFixed(6) + ", " + my_markers.route[my_markers.route.length-1].getPosition().lng.toFixed(6);
		callReverseGeocoder("target", my_markers.route[my_markers.route.length-1].getPosition().lat, my_markers.route[my_markers.route.length-1].getPosition().lng);
	}
}


function timeout_ReverseGeocoder() {
	//OSRM.debug.log("[timeout] reverse geocoder");
}

//prepare request and call reverse geocoder
function callReverseGeocoder(marker_id, lat, lon) {
	//build request
	if (marker_id == OSRM.SOURCE_MARKER_LABEL) {
		var src= OSRM.REVERSE_GEOCODE_POST + "&lat=" + lat + "&lon=" + lon;
		OSRM.JSONP.call( src, showReverseGeocoderResults_Source, timeout_ReverseGeocoder, OSRM.JSONP.TIMEOUT, "reverse_geocoder_source" );
		//OSRM.debug.log("[call2] reverse geocoder");
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL) {
		var src = OSRM.REVERSE_GEOCODE_POST + "&lat=" + lat + "&lon=" + lon;
		OSRM.JSONP.call( src, showReverseGeocoderResults_Target, timeout_ReverseGeocoder, OSRM.JSONP.TIMEOUT, "reverse_geocoder_target" );
	}
}
//processing JSONP response of reverse geocoder
//(with wrapper functions for source/target jsonp)
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
		if( response.address.city) {
			if( response.address.road)
				address += ", ";
			address += response.address.city;
		}
		if( address == "" )
			return;
		
		if(marker_id == OSRM.SOURCE_MARKER_LABEL)
			document.getElementById("input-source-name").value = address;
		else if(marker_id == OSRM.TARGET_MARKER_LABEL)
			document.getElementById("input-target-name").value = address;
	}
}


// prepare request and call geocoder
function callGeocoder(marker_id, query) {
	//geo coordinate given?
	if(query.match(/^\s*[-+]?[0-9]*\.?[0-9]+\s*[,;]\s*[-+]?[0-9]*\.?[0-9]+\s*$/)){
		var coord = query.split(/[,;]/);
		onclickGeocoderResult(marker_id, coord[0], coord[1]);
		return;
	}
	
	//build request
	if (marker_id == OSRM.SOURCE_MARKER_LABEL) {
		var src= OSRM.GEOCODE_POST + "&q=" + query;
		OSRM.JSONP.call( src, showGeocoderResults_Source, showGeocoderResults_Timeout, OSRM.JSONP.TIMEOUT, "geocoder_source" );
	} else if (marker_id == OSRM.TARGET_MARKER_LABEL) {
		var src = OSRM.GEOCODE_POST + "&q=" + query; 
		OSRM.JSONP.call( src, showGeocoderResults_Target, showGeocoderResults_Timeout, OSRM.JSONP.TIMEOUT, "geocoder_target" );		
	}
}


// helper function for clicks on geocoder search results
function onclickGeocoderResult(marker_id, lat, lon) {
	var index;
	if( marker_id == OSRM.SOURCE_MARKER_LABEL )
		index = my_markers.setSource( new L.LatLng(lat, lon) );
	else if( marker_id == OSRM.TARGET_MARKER_LABEL )
		index = my_markers.setTarget( new L.LatLng(lat, lon) );
	else
		index = -1;													// search via positions not yet implemented
	
	my_markers.route[index].show();
	my_markers.route[index].centerView();	
	getRoute(OSRM.FULL_DESCRIPTION);
}

// processing JSONP response of geocoder
// (with wrapper functions for source/target jsonp)
function showGeocoderResults_Source(response) {	showGeocoderResults(OSRM.SOURCE_MARKER_LABEL, response); }
function showGeocoderResults_Target(response) {	showGeocoderResults(OSRM.TARGET_MARKER_LABEL, response); }
function showGeocoderResults(marker_id, response) {
	if(response){
		if(response.length == 0) {
			showGeocoderResults_Empty();
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
function showGeocoderResults_Empty() {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("NO_RESULTS_FOUND")+".<p>";	
}
function showGeocoderResults_Timeout() {
	document.getElementById('information-box-headline').innerHTML = OSRM.loc("SEARCH_RESULTS")+":";
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("TIMED_OUT")+".<p>";	
}
