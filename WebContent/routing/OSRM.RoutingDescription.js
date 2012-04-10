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

// OSRM routing description
// [renders routing description and manages events]


OSRM.RoutingDescription = {
		
// route description events
onClickRouteDescription: function(geometry_index) {
	var positions = OSRM.G.route.getPositions();

	OSRM.G.markers.highlight.setPosition( positions[geometry_index] );
	OSRM.G.markers.highlight.show();
	OSRM.G.markers.highlight.centerView(OSRM.DEFAULTS.HIGHLIGHT_ZOOM_LEVEL);	
},
onClickCreateShortcut: function(src){
	src += '&z='+ OSRM.G.map.getZoom() + '&center=' + OSRM.G.map.getCenter().lat.toFixed(6) + ',' + OSRM.G.map.getCenter().lng.toFixed(6);
	OSRM.JSONP.call(OSRM.DEFAULTS.HOST_SHORTENER_URL+src, OSRM.RoutingDescription.showRouteLink, OSRM.RoutingDescription.showRouteLink_TimeOut, OSRM.DEFAULTS.JSONP_TIMEOUT, 'shortener');
	document.getElementById('route-link').innerHTML = '['+OSRM.loc("GENERATE_LINK_TO_ROUTE")+']';
},
showRouteLink: function(response){
	document.getElementById('route-link').innerHTML = '[<a class="result-link text-selectable" href="' +response.ShortURL+ '">'+response.ShortURL+'</a>]';
},
showRouteLink_TimeOut: function(){
	document.getElementById('route-link').innerHTML = '['+OSRM.loc("LINK_TO_ROUTE_TIMEOUT")+']';
},

// handling of routing description
show: function(response) {
	// compute query string
	var query_string = '?rebuild=1';
	for(var i=0; i<OSRM.G.markers.route.length; i++)
		query_string += '&loc=' + OSRM.G.markers.route[i].getLat().toFixed(6) + ',' + OSRM.G.markers.route[i].getLng().toFixed(6); 
 						
	// create link to the route
	var route_link ='[<a class="result-link" onclick="OSRM.RoutingDescription.onClickCreateShortcut(\'' + OSRM.DEFAULTS.WEBSITE_URL + query_string + '\')">'+OSRM.loc("GET_LINK_TO_ROUTE")+'</a>]';

	// create GPX link
	var gpx_link = '[<a class="result-link" onClick="document.location.href=\'' + OSRM.DEFAULTS.HOST_ROUTING_URL + query_string + '&output=gpx\';">'+OSRM.loc("GPX_FILE")+'</a>]';
		
	// create route description
	var route_desc = "";
	route_desc += '<table class="results-table medium-font">';

	for(var i=0; i < response.route_instructions.length; i++){
		//odd or even ?
		var rowstyle='results-odd';
		if(i%2==0) { rowstyle='results-even'; }

		route_desc += '<tr class="'+rowstyle+'">';
		
		route_desc += '<td class="result-directions">';
		route_desc += '<img width="18px" src="'+OSRM.G.images[OSRM.RoutingDescription.getDirectionIcon(response.route_instructions[i][0])].src+'" alt="" />';
		route_desc += "</td>";		
		
		route_desc += '<td class="result-items">';
		route_desc += '<div class="result-item" onclick="OSRM.RoutingDescription.onClickRouteDescription('+response.route_instructions[i][3]+')">';

//		// build route description
//		if( i == 0 )
//			route_desc += OSRM.loc("DIRECTION_"+response.route_instructions[i][0]).replace(/%s/, response.route_instructions[i][6]); 
//		else if( response.route_instructions[i][1] != "" )
//			route_desc += OSRM.loc("DIRECTION_"+response.route_instructions[i][0]).replace(/\[(.*)\]/,"");
//		else
//			route_desc += OSRM.loc("DIRECTION_"+response.route_instructions[i][0]).replace(/\[(.*)\]/,"$1").replace(/%s/, response.route_instructions[i][6]);
		
		route_desc += response.route_instructions[i][0];
		if( i == 0 )
			route_desc += ' ' + OSRM.loc( response.route_instructions[i][6] );		
		if( response.route_instructions[i][1] != "" ) {
			route_desc += ' on ';
			route_desc += '<b>' + response.route_instructions[i][1] + '</b>';
		}
		route_desc += '</div>';
		route_desc += "</td>";
		
		route_desc += '<td class="result-distance">';
		if( i != response.route_instructions.length-1 )
		route_desc += '<b>'+OSRM.Utils.metersToDistance(response.route_instructions[i][2])+'</b>';
		route_desc += "</td>";
		
		route_desc += "</tr>";
	}	
	route_desc += '</table>';
	
	// create header
	header = 
		'<div class="header-title">' + OSRM.loc("ROUTE_DESCRIPTION") + '</div>' +
		'<div class="full">' +
		'<div class="left">' +
		'<div class="header-content">' + OSRM.loc("DISTANCE")+": " + OSRM.Utils.metersToDistance(response.route_summary.total_distance) + '</div>' +
		'<div class="header-content">' + OSRM.loc("DURATION")+": " + OSRM.Utils.secondsToTime(response.route_summary.total_time) + '</div>' +
		'</div>' +
		'<div class="right">' +
		'<div id="route-link" class="header-content">' + route_link + '</div>' +
		'<div class="header-content">' + gpx_link + '</div>' +
		'</div>' +		
		'</div>';

	// update DOM
	document.getElementById('information-box-header').innerHTML = header;
	document.getElementById('information-box').innerHTML = route_desc;
},

// simple description
showSimple: function(response) {
	header = 
		'<div class="header-title">' + OSRM.loc("ROUTE_DESCRIPTION") + '</div>' +
		'<div class="full">' +
		'<div class="left">' +
		'<div class="header-content">' + OSRM.loc("DISTANCE")+": " + OSRM.Utils.metersToDistance(response.route_summary.total_distance) + '</div>' +
		'<div class="header-content">' + OSRM.loc("DURATION")+": " + OSRM.Utils.secondsToTime(response.route_summary.total_time) + '</div>' +
		'</div>' +
		'<div class="right">' +
		'</div>' +		
		'</div>';	

	// update DOM
	document.getElementById('information-box-header').innerHTML = header;
	document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED")+"</div>";	
},

// no description
showNA: function( display_text ) {
	header = 
		'<div class="header-title">' + OSRM.loc("ROUTE_DESCRIPTION") + '</div>' +
		'<div class="full">' +
		'<div class="left">' +
		'<div class="header-content">' + OSRM.loc("DISTANCE")+": N/A" + '</div>' +
		'<div class="header-content">' + OSRM.loc("DURATION")+": N/A" + '</div>' +
		'</div>' +
		'<div class="right">' +
		'</div>' +		
		'</div>';

	// update DOM
	document.getElementById('information-box-header').innerHTML = header;
	document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+display_text+"</div>";	
},

// map driving instructions to icons
// [TODO: language-safe implementation]
getDirectionIcon: function(name) {
	var directions = {
		"Turn left":"turn-left",
		"Turn right":"turn-right",
		"U-Turn":"u-turn",
		"Head":"continue",
		"Continue":"continue",
		"Turn slight left":"slight-left",
		"Turn slight right":"slight-right",
		"Turn sharp left":"sharp-left",
		"Turn sharp right":"sharp-right",
		"Enter roundabout and leave at first exit":"round-about",
		"Enter roundabout and leave at second exit":"round-about",
		"Enter roundabout and leave at third exit":"round-about",
		"Enter roundabout and leave at fourth exit":"round-about",
		"Enter roundabout and leave at fifth exit":"round-about",
		"Enter roundabout and leave at sixth exit":"round-about",
		"Enter roundabout and leave at seventh exit":"round-about",
		"Enter roundabout and leave at eighth exit":"round-about",
		"Enter roundabout and leave at nineth exit":"round-about",
		"Enter roundabout and leave at tenth exit":"round-about",
		"Enter roundabout and leave at one of the too many exit":"round-about",
		"You have reached your destination":"target"
	};
	
	if( directions[name] )
		return directions[name];
	else
		return "default";
}

};