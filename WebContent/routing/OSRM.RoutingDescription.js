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
	var query_string = '?hl=' + OSRM.Localization.current_language;
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
		route_desc += '<img width="18px" src="'+OSRM.RoutingDescription.getDrivingInstructionIcon(response.route_instructions[i][0])+'" alt="" />';
		route_desc += "</td>";		
		
		route_desc += '<td class="result-items">';
		route_desc += '<div class="result-item" onclick="OSRM.RoutingDescription.onClickRouteDescription('+response.route_instructions[i][3]+')">';

		// build route description
		if( i == 0 )
			route_desc += OSRM.loc(OSRM.RoutingDescription.getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"$1").replace(/%s/, OSRM.loc(response.route_instructions[i][6]) );
		else if( response.route_instructions[i][1] != "" )
			route_desc += OSRM.loc(OSRM.RoutingDescription.getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"$1").replace(/%s/, response.route_instructions[i][1]);
		else
			route_desc += OSRM.loc(OSRM.RoutingDescription.getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"");

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

// retrieve driving instruction icon from instruction id
getDrivingInstructionIcon: function(server_instruction_id) {
	var local_icon_id = "direction_";
	server_instruction_id = server_instruction_id.replace(/^11-\d{1,}$/,"11");		// dumb check, if there is a roundabout (all have the same icon)
	local_icon_id += server_instruction_id;
	
	if( OSRM.G.images[local_icon_id] )
		return OSRM.G.images[local_icon_id].getAttribute("src");
	else
		return OSRM.G.images["direction_0"].getAttribute("src");
},

// retrieve driving instructions from instruction ids
getDrivingInstruction: function(server_instruction_id) {
	var local_instruction_id = "DIRECTION_";
	server_instruction_id = server_instruction_id.replace(/^11-\d{2,}$/,"11-x");	// dumb check, if there are 10+ exits on a roundabout (say the same for exit 10+)
	local_instruction_id += server_instruction_id;
	
	var description = OSRM.loc( local_instruction_id );
	if( description == local_instruction_id)
		return OSRM.loc("DIRECTION_0");
	return description;
}

};