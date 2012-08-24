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

// OSRM printing
// [printing support]


OSRM.Printing = {
		
// directory with printing code, and base OSRM directory relative to this directory
DIRECTORY: 'printing/',
BASE_DIRECTORY: '../',
		
// create UI for printing in mainwindow
init: function() {
	var icon = document.createElement('div');
	icon.id = "gui-printer-inactive";
	icon.className = "iconic-button top-right-button";
	
	var spacer = document.createElement('div');
	spacer.className = "quad top-right-button";
	
	var input_mask_header = document.getElementById('input-mask-header'); 
	input_mask_header.appendChild(spacer,input_mask_header.lastChild);
	input_mask_header.appendChild(icon,input_mask_header.lastChild);
	
	document.getElementById("gui-printer-inactive").onclick = OSRM.Printing.openPrintWindow;
	
	OSRM.Browser.onUnloadHandler( OSRM.Printing.uninit );	
},
uninit: function() {
	if(OSRM.G.printwindow)
		OSRM.G.printwindow.close();
},
		

// switch printer button on/off
activate: function() {		// use showing route description as trigger
	if( document.getElementById("gui-printer-inactive") )
		document.getElementById("gui-printer-inactive").id = "gui-printer";
},
deactivate: function() {	// use hide route as trigger
	if( document.getElementById("gui-printer") )
		document.getElementById("gui-printer").id = "gui-printer-inactive";
},		


// create UI in printwindow
show: function(response) {
	// create header
	var header;
	if( OSRM.Browser.IE6_8 ) {	// tables used for compatibility with legacy IE (quirks mode)
		header =
		'<thead class="description-header"><tr><td colspan="3">' +
		
		'<table class="full">' +
		'<tr class="row">' +
		
		'<td class="left stretch">' +
		'<table class="full">' +
		'<tr class="row">' +
		'<td class="left description-header-label">' + OSRM.loc("GUI_START")+ ': </td>' +
		'<td class="left description-header-content stretch">' + document.getElementById("gui-input-source").value + '</td>' +
		'</tr>' +
		'<tr class="row">' +
		'<td class="left description-header-label">' + OSRM.loc("GUI_END")+ ': </td>' +
		'<td class="left description-header-content stretch">' + document.getElementById("gui-input-target").value + '</td>' +
		'</tr>' +		
		'</table>' +
		'</td>' +
		
		'<td class="left">' +
		'<table class="full">' +
		'<tr class="row">' +
		'<td class="left description-header-label">' +  OSRM.loc("DISTANCE")+': </td>' +
		'<td class="left description-header-content">' + OSRM.Utils.toHumanDistance(response.route_summary.total_distance) + '</td>' +
		'</tr>' +
		'<tr class="row">' +
		'<td class="left description-header-label">' +  OSRM.loc("DURATION")+': </td>' +
		'<td class="left description-header-content">' + OSRM.Utils.toHumanTime(response.route_summary.total_time) + '</td>' +
		'</tr>' +
		'</table>' +
		'</td>' +
		
		'</tr>' +
		'</table>' +
		
		'<div class="quad"></div>' + 
		'</td></tr></thead>';
	} else {
		header = 
		'<thead class="description-header"><tr><td colspan="3">' +
		
		'<div class="full">' +
		'<div class="row">' +
		
		'<div class="left stretch">' +
		'<div class="full">' +
		'<div class="row">' +
		'<div class="left description-header-label">' + OSRM.loc("GUI_START")+ ': </div>' +
		'<div class="left description-header-content stretch">' + document.getElementById("gui-input-source").value + '</div>' +
		'</div>' +
		'<div class="row">' +
		'<div class="left description-header-label">' + OSRM.loc("GUI_END")+ ': </div>' +
		'<div class="left description-header-content stretch">' + document.getElementById("gui-input-target").value + '</div>' +
		'</div>' +		
		'</div>' +
		'</div>' +
		
		'<div class="left">' +
		'<div class="full">' +
		'<div class="row">' +
		'<div class="left description-header-label">' +  OSRM.loc("DISTANCE")+': </div>' +
		'<div class="left description-header-content">' + OSRM.Utils.toHumanDistance(response.route_summary.total_distance) + '</div>' +
		'</div>' +
		'<div class="row">' +
		'<div class="left description-header-label">' +  OSRM.loc("DURATION")+': </div>' +
		'<div class="left description-header-content">' + OSRM.Utils.toHumanTime(response.route_summary.total_time) + '</div>' +
		'</div>' +
		'</div>' +
		'</div>' +
		
		'</div>' +
		'</div>' +
		
		'<div class="quad"></div>' + 
		'</td></tr></thead>';
	}

	// create route description
	var body = '<tbody class="description-body">';
	for(var i=0; i < response.route_instructions.length; i++){
		//odd or even ?
		var rowstyle='description-body-odd';
		if(i%2==0) { rowstyle='description-body-even'; }

		body += '<tr class="'+rowstyle+'">';
		
		body += '<td class="description-body-directions">';
		body += '<img class="description-body-direction" src="'+OSRM.Printing.BASE_DIRECTORY+OSRM.RoutingDescription._getDrivingInstructionIcon(response.route_instructions[i][0])+'" alt="" />';
		body += "</td>";		

		// build route description
		body += '<td class="description-body-items">';
		if( response.route_instructions[i][1] != "" )
			body += OSRM.loc(OSRM.RoutingDescription._getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"$1").replace(/%s/, response.route_instructions[i][1]).replace(/%d/, OSRM.loc(response.route_instructions[i][6]));
		else
			body += OSRM.loc(OSRM.RoutingDescription._getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"").replace(/%d/, OSRM.loc(response.route_instructions[i][6]));
		body += "</td>";
		
		body += '<td class="description-body-distance">';
		body += (i == response.route_instructions.length-1) ? '&nbsp;' : '<b>'+OSRM.Utils.toHumanDistance(response.route_instructions[i][2])+'</b>';	// fill last entry with a space
		body += "</td>";
		
		body += "</tr>";
	}
	body += '</tbody>';
	
	// put everything in DOM
	var print_window = OSRM.G.printwindow;	
	print_window.document.getElementById('description').innerHTML = '<table class="description medium-font">' + header + body + '</table>';		
	print_window.document.getElementById('overview-map-description').innerHTML = '<table class="description medium-font">' + header + '</table>';
	
	// draw map
	var tile_server_id = OSRM.G.map.getActiveLayerId();
	var positions = OSRM.G.route.getPositions();	
	var bounds = new L.LatLngBounds( positions );	
	var zoom = print_window.OSRM.drawMap( OSRM.DEFAULTS.TILE_SERVERS[tile_server_id], bounds );

	// draw markers
	print_window.OSRM.prefetchIcons( OSRM.G.images );
	print_window.OSRM.drawMarkers( OSRM.G.markers.route );
	
	// draw route & query for better geometry
	print_window.OSRM.drawRoute( positions );
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&z='+zoom+'&instructions=false', OSRM.Printing.drawRoute, OSRM.Printing.timeoutRoute, OSRM.DEFAULTS.JSONP_TIMEOUT, 'print');
	// NOTE: correct zoom level was appended as second zoom parameter to JSONP call -> OSRM API only considers the last one! 
},
timeoutRoute: function() {},
drawRoute: function(response) {
	if(!response)
		return;

	response.alternative_geometries.unshift( response.route_geometry );
	if( OSRM.G.active_alternative >= response.alternative_geometries.length )	// no redraw if the selected alternative cannot be found
		return;
	positions = OSRM.RoutingGeometry._decode( response.alternative_geometries[ OSRM.G.active_alternative ], 5 );
	OSRM.G.printwindow.OSRM.drawRoute( positions );
},


// opens the print window and closes old instances
openPrintWindow: function() {
	// do not open window if there is no route to draw (should never trigger!)
	if( !OSRM.G.route.isRoute() || !OSRM.G.route.isShown() )
		return;
	
	// close old window (should we really do this?)
	if( OSRM.G.printwindow )
		OSRM.G.printwindow.close();
	
	// generate a new window and wait till it has finished loading
	OSRM.G.printwindow = window.open( OSRM.Printing.DIRECTORY + "printing.html","","width=540,height=500,left=100,top=100,dependent=yes,location=no,menubar=no,scrollbars=yes,status=no,toolbar=no,resizable=yes");
},


// add content to printwindow after it has finished loading
printWindowLoaded: function(){
	var print_window = OSRM.G.printwindow;
	var print_document = print_window.document;
	
	// add css images
	var css_list = [
                	{ id:'#gui-printer-inactive',		image_id:'printer_inactive'},
                	{ id:'#gui-printer',				image_id:'printer'},
                	{ id:'#gui-printer:hover',			image_id:'printer_hover'},
                	{ id:'#gui-printer:active',			image_id:'printer_active'}
                ];
	var stylesheet = OSRM.CSS.getStylesheet("printing.css", print_document);
	for(var i=0; i<css_list.length; i++) {
		OSRM.CSS.insert( stylesheet, css_list[i].id, "background-image:url("+ OSRM.Printing.BASE_DIRECTORY + OSRM.G.images[css_list[i].image_id].getAttribute("src") + ");" );
	}
	
	// scale control
	print_window.OSRM.G.active_distance_format = OSRM.G.active_distance_format;
	
	// localization 
	print_window.OSRM.G.Localization.culture = OSRM.loc("CULTURE"); 
	print_document.getElementById('description-label').innerHTML = OSRM.loc( "ROUTE_DESCRIPTION" );
	print_document.getElementById('overview-map-label').innerHTML = OSRM.loc( "OVERVIEW_MAP" );	
	if( !OSRM.G.route.isRoute() || !OSRM.G.route.isShown() ) {		// error message if no route available (can trigger if user refreshes print-window)
		print_document.getElementById("description").innerHTML = OSRM.loc("NO_ROUTE_SELECTED");
		return;	
	}
	
	// add routing content
	OSRM.Printing.show( OSRM.G.response );
	
	// add events
	print_document.getElementById("gui-printer-inactive").id = "gui-printer";	
	print_document.getElementById('gui-printer').onclick = print_window.printWindow;	
	
	// finally, focus on printwindow
	print_window.focus();
}

};