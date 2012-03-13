// some constants
OSRM.GEOCODE_POST = 'http://nominatim.openstreetmap.org/search?format=json';
OSRM.SOURCE_MARKER_LABEL = "source";
OSRM.TARGET_MARKER_LABEL = "target";


// prepare request and call geocoder
function callGeocoder(marker_id, query) {
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
				html += '<div class="result-item" onclick="onclickGeocoderResult('+marker_id+', '+result.lat+', '+result.lon+');">'+result.display_name+'</div>';
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
