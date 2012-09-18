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
		
// directory with qrcodes files
QR_DIRECTORY: 'qrcodes/',
		
// initialization of required variables and events
init: function() {
	OSRM.G.active_shortlink = null;
	OSRM.Browser.onUnloadHandler( OSRM.RoutingDescription.uninit );
},
uninit: function() {
	if( OSRM.G.qrcodewindow )
		OSRM.G.qrcodewindow.close();	
},

// route description events
onMouseOverRouteDescription: function(lat, lng) {
	OSRM.G.markers.hover.setPosition( new L.LatLng(lat, lng) );
	OSRM.G.markers.hover.show();

},
onMouseOutRouteDescription: function(lat, lng) {
	OSRM.G.markers.hover.hide();	
},
onClickRouteDescription: function(lat, lng, desc) {
	OSRM.G.markers.highlight.setPosition( new L.LatLng(lat, lng) );
	OSRM.G.markers.highlight.show();
	OSRM.G.markers.highlight.centerView(OSRM.DEFAULTS.HIGHLIGHT_ZOOM_LEVEL);	
	
	OSRM.G.markers.highlight.description = desc;	
	document.getElementById("description-"+desc).className = "description-body-item description-body-item-selected";
},
onClickCreateShortcut: function(src){
	src += '&z='+ OSRM.G.map.getZoom() + '&center=' + OSRM.G.map.getCenter().lat.toFixed(6) + ',' + OSRM.G.map.getCenter().lng.toFixed(6);
	src += '&alt='+OSRM.G.active_alternative;
	src += '&df=' + OSRM.G.active_distance_format;
	src += '&re=' + OSRM.G.active_routing_engine;
	
	var source = OSRM.DEFAULTS.SHORTENER_PARAMETERS.replace(/%url/, OSRM.DEFAULTS.HOST_SHORTENER_URL+src); 
	
	OSRM.JSONP.call(source, OSRM.RoutingDescription.showRouteLink, OSRM.RoutingDescription.showRouteLink_TimeOut, OSRM.DEFAULTS.JSONP_TIMEOUT, 'shortener');
	document.getElementById('route-link').innerHTML = '['+OSRM.loc("GENERATE_LINK_TO_ROUTE")+']';
},
showRouteLink: function(response){
	if(!response || !response[OSRM.DEFAULTS.SHORTENER_REPLY_PARAMETER]) {
		OSRM.RoutingDescription.showRouteLink_TimeOut();
		return;
	}
	
	OSRM.G.active_shortlink = response[OSRM.DEFAULTS.SHORTENER_REPLY_PARAMETER];
	document.getElementById('route-link').innerHTML =
		'[<a class="route-link" onClick="OSRM.RoutingDescription.showQRCode();">'+OSRM.loc("QR")+'</a>]' + ' ' +
		'[<a class="route-link" href="' +OSRM.G.active_shortlink+ '">'+OSRM.G.active_shortlink.substring(7)+'</a>]';
},
showRouteLink_TimeOut: function(){
	document.getElementById('route-link').innerHTML = '['+OSRM.loc("LINK_TO_ROUTE_TIMEOUT")+']';
},
showQRCode: function(response){
	if( OSRM.G.qrcodewindow )
		OSRM.G.qrcodewindow.close();	
	OSRM.G.qrcodewindow = window.open( OSRM.RoutingDescription.QR_DIRECTORY+"qrcodes.html","","width=280,height=250,left=100,top=100,dependent=yes,location=no,menubar=no,scrollbars=no,status=no,toolbar=no,resizable=no");
},

// handling of routing description
show: function(response) {
	// activate GUI features that need a route
	OSRM.GUI.activateRouteFeatures();
	
	// compute query string
	var query_string = '?hl=' + OSRM.Localization.current_language;
	for(var i=0; i<OSRM.G.markers.route.length; i++)
		query_string += '&loc=' + OSRM.G.markers.route[i].getLat().toFixed(6) + ',' + OSRM.G.markers.route[i].getLng().toFixed(6); 
 						
	// create link to the route
	var route_link ='[<a class="route-link" onclick="OSRM.RoutingDescription.onClickCreateShortcut(\'' + OSRM.DEFAULTS.WEBSITE_URL + query_string + '\')">'+OSRM.loc("GET_LINK_TO_ROUTE")+'</a>]';

	// create GPX link
	var gpx_link = '[<a class="route-link" onClick="document.location.href=\'' + OSRM.G.active_routing_server_url + query_string + '&output=gpx\';">'+OSRM.loc("GPX_FILE")+'</a>]';
	
	// check highlight marker to get id of corresponding description
	// [works as changing language or metric does not remove the highlight marker!]	
	var selected_description = null;
	if( OSRM.G.markers.highlight.isShown() )
		selected_description = OSRM.G.markers.highlight.description;
		
	// create route description
	var positions = OSRM.G.route.getPositions();
	var body = "";
	body += '<table class="description medium-font">';
	for(var i=0; i < response.route_instructions.length; i++){
		//odd or even ?
		var rowstyle='description-body-odd';
		if(i%2==0) { rowstyle='description-body-even'; }

		body += '<tr class="'+rowstyle+'">';
		
		body += '<td class="description-body-directions">';
		body += '<img class="description-body-direction" src="'+ OSRM.RoutingDescription._getDrivingInstructionIcon(response.route_instructions[i][0]) + '" alt=""/>';		
		body += '</td>';
		
		body += '<td class="description-body-items">';
		var pos = positions[response.route_instructions[i][3]];
		body += '<div id="description-'+i+'" class="description-body-item '+(selected_description==i ? "description-body-item-selected": "")+'" ' +
			'onclick="OSRM.RoutingDescription.onClickRouteDescription('+pos.lat.toFixed(6)+","+pos.lng.toFixed(6)+","+i+')" ' +
			'onmouseover="OSRM.RoutingDescription.onMouseOverRouteDescription('+pos.lat.toFixed(6)+","+pos.lng.toFixed(6)+')" ' +
			'onmouseout="OSRM.RoutingDescription.onMouseOutRouteDescription('+pos.lat.toFixed(6)+","+pos.lng.toFixed(6)+')">';

		// build route description
		if( response.route_instructions[i][1] != "" )
			body += OSRM.loc(OSRM.RoutingDescription._getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"$1").replace(/%s/, response.route_instructions[i][1]).replace(/%d/, OSRM.loc(response.route_instructions[i][6]));
		else
			body += OSRM.loc(OSRM.RoutingDescription._getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"").replace(/%d/, OSRM.loc(response.route_instructions[i][6]));

		body += '</div>';
		body += "</td>";
		
		body += '<td class="description-body-distance">';
		if( i != response.route_instructions.length-1 )
		body += '<b>'+OSRM.Utils.toHumanDistance(response.route_instructions[i][2])+'</b>';
		body += "</td>";
		
		body += "</tr>";
	}	
	body += '</table>';
	
	// build header
	header = OSRM.RoutingDescription._buildHeader(OSRM.Utils.toHumanDistance(response.route_summary.total_distance), OSRM.Utils.toHumanTime(response.route_summary.total_time), route_link, gpx_link);

	// update DOM
	document.getElementById('information-box-header').innerHTML = header;
	document.getElementById('information-box').innerHTML = body;
	
	// add alternative GUI (has to be done last since DOM has to be updated before events are registered)
	OSRM.RoutingAlternatives.show();	
},

// simple description
showSimple: function(response) {
	// build header
	header = OSRM.RoutingDescription._buildHeader(OSRM.Utils.toHumanDistance(response.route_summary.total_distance), OSRM.Utils.toHumanTime(response.route_summary.total_time), "", "");

	// update DOM
	document.getElementById('information-box-header').innerHTML = header;
	document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED")+"</div>";	
},

// no description
showNA: function( display_text ) {
	// build header
	header = OSRM.RoutingDescription._buildHeader("N/A", "N/A", "", "");

	// update DOM
	document.getElementById('information-box-header').innerHTML = header;
	document.getElementById('information-box').innerHTML = "<div class='no-results big-font'>"+display_text+"</div>";	
},

// build header
_buildHeader: function(distance, duration, route_link, gpx_link) {
	var temp = 
		'<div class="header-title">' + OSRM.loc("ROUTE_DESCRIPTION") + '</div>' +
		
		'<div class="full">' +
		'<div class="row">' +

		'<div class="left">' +
		'<div class="full">' +
		'<div class="row">' +
		'<div class="left header-label">' + OSRM.loc("DISTANCE")+":" + '</div>' +
		'<div class="left header-content stretch">' + distance + '</div>' +
		'</div>' +
		'<div class="row">' +
		'<div class="left header-label">' + OSRM.loc("DURATION")+":" + '</div>' +
		'<div class="left header-content stretch">' + duration + '</div>' +
		'</div>' +
		'</div>' +
		'</div>' +
		
		'<div class="left">' +
		'<div class="full">' +
		'<div class="row">' +
		'<div class="right header-content" id="route-link">' + route_link + '</div>' +
		'</div>' +
		'<div class="row">' +
		'<div class="right header-content">' + gpx_link + '</div>' +
		'</div>' +
		'</div>' +
		'</div>' +
		
		'</div>' +
		'</div>' +		
		
		'</div>';	
//		'<div class="header-title">' + OSRM.loc("ROUTE_DESCRIPTION") + '</div>' +
//		'<div class="full">' +
//		'<div class="row">' +
//		'<div class="left header-label">' + OSRM.loc("DISTANCE")+":" + '</div>' +
//		'<div class="left header-content">' + distance + '</div>' +
//		'<div class="right header-content" id="route-link">' + route_link + '</div>' +
//		'</div>' +
//		'<div class="row">' +
//		'<div class="left header-label">' + OSRM.loc("DURATION")+":" + '</div>' +
//		'<div class="left header-content">' + duration + '</div>' +
//		'<div class="right header-content">' + gpx_link + '</div>' +
//		'</div>' +
//		'</div>';
	return temp;
},

// retrieve driving instruction icon from instruction id
_getDrivingInstructionIcon: function(server_instruction_id) {
	var local_icon_id = "direction_";
	server_instruction_id = server_instruction_id.replace(/^11-\d{1,}$/,"11");		// dumb check, if there is a roundabout (all have the same icon)
	local_icon_id += server_instruction_id;
	
	if( OSRM.G.images[local_icon_id] )
		return OSRM.G.images[local_icon_id].getAttribute("src");
	else
		return OSRM.G.images["direction_0"].getAttribute("src");
},

// retrieve driving instructions from instruction ids
_getDrivingInstruction: function(server_instruction_id) {
	var local_instruction_id = "DIRECTION_";
	server_instruction_id = server_instruction_id.replace(/^11-\d{2,}$/,"11-x");	// dumb check, if there are 10+ exits on a roundabout (say the same for exit 10+)
	local_instruction_id += server_instruction_id;
	
	var description = OSRM.loc( local_instruction_id );
	if( description == local_instruction_id)
		return OSRM.loc("DIRECTION_0");
	return description;
}

};