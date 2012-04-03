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
	src += '&z='+ OSRM.G.map.getZoom() + '&center=' + OSRM.G.map.getCenter().lat + ',' + OSRM.G.map.getCenter().lng;
	OSRM.JSONP.call(OSRM.DEFAULTS.HOST_SHORTENER_URL+src, OSRM.RoutingDescription.showRouteLink, OSRM.RoutingDescription.showRouteLink_TimeOut, OSRM.DEFAULTS.JSONP_TIMEOUT, 'shortener');
	document.getElementById('route-prelink').innerHTML = '['+OSRM.loc("GENERATE_LINK_TO_ROUTE")+']';
},
showRouteLink: function(response){
	document.getElementById('route-prelink').innerHTML = '[<a id="gpx-link" class = "text-selectable" href="' +response.ShortURL+ '">'+response.ShortURL+'</a>]';
// 	document.getElementById('route-prelink').innerHTML = '[<input class="text-selectable output-box" style="border:none" value="'+response.ShortURL+'" type="text" onfocus="this.select()" readonly="readonly"/>]';
},
showRouteLink_TimeOut: function(){
	document.getElementById('route-prelink').innerHTML = '['+OSRM.loc("LINK_TO_ROUTE_TIMEOUT")+']';
},

// handling of routing description
show: function(response) {
	// compute query string
	var query_string = '?rebuild=1';
	for(var i=0; i<OSRM.G.markers.route.length; i++)
		query_string += '&loc=' + OSRM.G.markers.route[i].getLat() + ',' + OSRM.G.markers.route[i].getLng(); 
 						
	// create link to the route
	var route_link ='<span class="route-summary" id="route-prelink">[<a id="gpx-link" onclick="OSRM.RoutingDescription.onClickCreateShortcut(\'' + OSRM.DEFAULTS.WEBSITE_URL + query_string + '\')">'+OSRM.loc("GET_LINK_TO_ROUTE")+'</a>]</span>';

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
		route_desc += '<img width="18px" src="images/'+OSRM.RoutingDescription.getDirectionIcon(response.route_instructions[i][0])+'" alt="" />';
		route_desc += "</td>";		
		
		route_desc += '<td class="result-items">';
		route_desc += '<span class="result-item" onclick="OSRM.RoutingDescription.onClickRouteDescription('+response.route_instructions[i][3]+')">';
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
		route_desc += '<b>'+OSRM.Utils.metersToDistance(response.route_instructions[i][2])+'</b>';
		route_desc += "</td>";
		
		route_desc += "</tr>";
	}	
		
	route_desc += '</table>';		
	headline = "";
	headline += OSRM.loc("ROUTE_DESCRIPTION")+":<br>";
	headline += '<div style="float:left;width:40%">';
	headline += "<span class='route-summary'>"
		+ OSRM.loc("DISTANCE")+": " + OSRM.Utils.metersToDistance(response.route_summary.total_distance)
		+ "<br>"
		+ OSRM.loc("DURATION")+": " + OSRM.Utils.secondsToTime(response.route_summary.total_time)
		+ "</span>";		
	headline +=	'</div>';
	headline += '<div style="float:left;text-align:right;width:60%;">'+route_link+'<br>'+gpx_link+'</div>';

	var output = "";
	output += route_desc;

	document.getElementById('information-box-headline').innerHTML = headline;
	document.getElementById('information-box').innerHTML = output;
},

// simple description
showSimple: function(response) {
	headline = OSRM.loc("ROUTE_DESCRIPTION")+":<br>";
	headline += "<span class='route-summary'>"
			+ OSRM.loc("DISTANCE")+": " + OSRM.Utils.metersToDistance(response.route_summary.total_distance)
			+ "<br>"
			+ OSRM.loc("DURATION")+": " + OSRM.Utils.secondsToTime(response.route_summary.total_time)
			+ "</span>";
	headline += '<br><br>';

	document.getElementById('information-box-headline').innerHTML = headline;
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED")+".<p>";	
},

// no description
showNA: function( display_text ) {
	headline = OSRM.loc("ROUTE_DESCRIPTION")+":<br>";
	headline += "<span class='route-summary'>"
			+ OSRM.loc("DISTANCE")+": N/A"
			+ "<br>"
			+ OSRM.loc("DURATION")+": N/A"
			+ "</span>";
	headline += '<br><br>';

	document.getElementById('information-box-headline').innerHTML = headline;
	document.getElementById('information-box').innerHTML = "<br><p style='font-size:14px;font-weight:bold;text-align:center;'>"+display_text+".<p>";	
},

// map driving instructions to icons
// [TODO: language-safe implementation]
getDirectionIcon: function(name) {
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

};