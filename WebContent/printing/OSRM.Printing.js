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
		
// create UI for printing in mainwindow
init: function() {
	var icon = document.createElement('div');
	icon.id = "gui-printer";
	icon.className = "iconic-button top-right-button";
	
	var spacer = document.createElement('div');
	spacer.className = "quad top-right-button";
	
	var input_mask_header = document.getElementById('input-mask-header'); 
	input_mask_header.appendChild(spacer,input_mask_header.lastChild);
	input_mask_header.appendChild(icon,input_mask_header.lastChild);
	
	document.getElementById("gui-printer").onclick = OSRM.Printing.print;	
},
		

// create UI in printwindow
show: function(response) {
	// create header
	var header = 
		'<thead class="header-group"><tr><td colspan="3">' +
		
		'<div class="full">' +
		'<div style="display:table-row">' +
		
		'<div class="left">' +
		'<div class="header-content" style="margin:0px 2px 0px 0px;">' + OSRM.loc("GUI_START")+ ': </div>' +
		'<div class="header-content" style="margin:0px 2px 0px 0px;">' + OSRM.loc("GUI_END")+ ': </div>' +
		'</div>' +
		
		'<div class="left" style="width:100%">' +
		'<div class="header-content" style="font-weight:bold;">' + document.getElementById("gui-input-source").value + '</div>' +
		'<div class="header-content" style="font-weight:bold;">' + document.getElementById("gui-input-target").value + '</div>' +
		'</div>' +
		
		'<div class="left">' +
		'<div class="header-content" style="margin:0px 2px 0px 0px;">' + OSRM.loc("DISTANCE")+': </div>' +
		'<div class="header-content" style="margin:0px 2px 0px 0px;">' + OSRM.loc("DURATION")+': </div>' +
		'</div>' +		
		
		'<div class="left">' +
		'<div class="header-content" style="font-weight:bold;">' + OSRM.Utils.metersToDistance(response.route_summary.total_distance) + '</div>' +
		'<div class="header-content" style="font-weight:bold;">' + OSRM.Utils.secondsToTime(response.route_summary.total_time) + '</div>' +
		'</div>' +
		
		'</div>' +
		'</div>' +
		
		'<div class="quad"></div>' + 
		'</td></tr></thead>';	
	
	// create route description
	var route_desc = '<tbody class="row-group">';
	for(var i=0; i < response.route_instructions.length; i++){
		//odd or even ?
		var rowstyle='results-odd';
		if(i%2==0) { rowstyle='results-even'; }

		route_desc += '<tr class="'+rowstyle+'">';
		
		route_desc += '<td class="result-directions">';
		route_desc += '<img width="18px" src="../'+OSRM.RoutingDescription.getDrivingInstructionIcon(response.route_instructions[i][0])+'" alt="" />';
		route_desc += "</td>";		
		
		route_desc += '<td class="result-items">';
		route_desc += '<div class="result-item">';

		// build route description
		if( response.route_instructions[i][1] != "" )
			route_desc += OSRM.loc(OSRM.RoutingDescription.getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"$1").replace(/%s/, response.route_instructions[i][1]).replace(/%d/, OSRM.loc(response.route_instructions[i][6]));
		else
			route_desc += OSRM.loc(OSRM.RoutingDescription.getDrivingInstruction(response.route_instructions[i][0])).replace(/\[(.*)\]/,"").replace(/%d/, OSRM.loc(response.route_instructions[i][6]));

		route_desc += '</div>';
		route_desc += "</td>";
		
		route_desc += '<td class="result-distance">';
		if( i != response.route_instructions.length-1 )
		route_desc += '<b>'+OSRM.Utils.metersToDistance(response.route_instructions[i][2])+'</b>';
		route_desc += "</td>";
		
		route_desc += "</tr>";
	}
	route_desc += '</tbody>';
	
	// put everything in DOM
	OSRM.G.printwindow.document.getElementById('description').innerHTML = '<table class="results-table medium-font">' + header + route_desc + '</table>';		
	OSRM.G.printwindow.document.getElementById('overview-map-description').innerHTML = '<table class="results-table medium-font">' + header + '</table>';
	
	// draw map
	var tile_server_id = OSRM.MapView.getActiveLayerId();
	var map = OSRM.G.printwindow.initialize( OSRM.DEFAULTS.TILE_SERVERS[tile_server_id] );
	// draw markers
	var markers = OSRM.G.markers.route;
	map.addLayer( new L.MouseMarker( markers[0].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-source']} ) );
	for(var i=1, size=markers.length-1; i<size; i++)
		map.addLayer( new L.MouseMarker( markers[i].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-via']} ) );
	map.addLayer( new L.MouseMarker( markers[markers.length-1].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-target']} ));
	// draw route
	OSRM.Printing.route = new L.DashedPolyline();
	var route = OSRM.Printing.route; 
	route.setLatLngs( OSRM.G.route.getPositions() );
	route.setStyle( {dashed:false,clickable:false,color:'#0033FF', weight:5} );
	map.addLayer( route );
	var bounds = new L.LatLngBounds( OSRM.G.route.getPositions() );
	map.fitBoundsUI( bounds );
	// query for a better route geometry
	var zoom = map.getBoundsZoom(bounds);
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&z='+zoom+'&instructions=false', OSRM.Printing.drawRoute, OSRM.Printing.timeoutRoute, OSRM.DEFAULTS.JSONP_TIMEOUT, 'print');
},
timeoutRoute: function() {},
drawRoute: function(response) {
	if(!response)
		return;
	var geometry = OSRM.RoutingGeometry._decode(response.route_geometry, 5);
	OSRM.Printing.route.setLatLngs( geometry );
},


//open printWindow
print: function() {
	// do not open window if there is no route to draw
	if( !OSRM.G.route.isRoute() || !OSRM.G.route.isShown() )
		return;
	
	// close old window (should we really do this?)
	if( OSRM.G.printwindow )
		OSRM.G.printwindow.close();
	
	// generate a new window and wait till it has finished loading
	OSRM.G.printwindow = window.open("printing/printing.html","","width=540,height=500,left=100,top=100,dependent=yes,location=no,menubar=no,scrollbars=yes,status=no,toolbar=no,resizable=yes");
	OSRM.Browser.onLoadHandler( OSRM.Printing.printwindowLoaded, OSRM.G.printwindow );
},


//add content to printwindow after it has finished loading
printwindowLoaded: function(){
	var print_window = OSRM.G.printwindow;
	var print_document = print_window.document;
	
	// add events
	print_document.getElementById('gui-printer').onclick = print_window.printWindow;
	
	// localization 
	print_document.getElementById('description-label').innerHTML = OSRM.loc( "ROUTE_DESCRIPTION" );
	print_document.getElementById('overview-map-label').innerHTML = OSRM.loc( "OVERVIEW_MAP" );

	// add routing content	
	OSRM.Printing.show( OSRM.G.response );
	
	// finally, focus on printwindow
	print_window.focus();
}

};