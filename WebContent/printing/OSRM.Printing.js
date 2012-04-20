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

// OSRM printer
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
	// add events
	OSRM.printwindow.document.getElementById('gui-printer').onclick = OSRM.printwindow.printWindow;
	
	// localization 
	OSRM.printwindow.document.getElementById('description-label').innerHTML = OSRM.loc( "ROUTE_DESCRIPTION" );
	OSRM.printwindow.document.getElementById('overview-map-label').innerHTML = OSRM.loc( "OVERVIEW_MAP" );

	// create header
	header = 
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
		'<div class="quad"></div>';	
	
	// create route description
	var route_desc = '';
	route_desc += '<table id="thetable" class="results-table medium-font">';
	route_desc += '<thead style="display:table-header-group;"><tr><td colspan="3">'+header+'</td></tr></thead>';
	route_desc += '<tbody stlye="display:table-row-group">';

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
	route_desc += '</tbody>';
	route_desc += '</table>';
	
	
	
	// put everything in DOM
	OSRM.printwindow.document.getElementById('description').innerHTML = route_desc;
	OSRM.printwindow.document.getElementById('overview-description').innerHTML = 
		'<table id="" class="results-table medium-font">' +
		'<thead style="display:table-header-group;"><tr><td colspan="3">'+header+'</td></tr></thead>'+
		'</table>';
	
	// init map
	OSRM.Printing.map = OSRM.printwindow.initialize( OSRM.DEFAULTS.TILE_SERVERS[0] );
	var map = OSRM.Printing.map;
	var markers = OSRM.G.markers.route;
	map.addLayer( new L.MouseMarker( markers[0].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-source']} ) );
	for(var i=1, size=markers.length-1; i<size; i++)
		map.addLayer( new L.MouseMarker( markers[i].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-via']} ) );
	map.addLayer( new L.MouseMarker( markers[markers.length-1].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-target']} ));
	
	OSRM.Printing.route = new L.DashedPolyline();
	var route = OSRM.Printing.route; 
	route.setLatLngs( OSRM.G.route.getPositions() );
	route.setStyle( {dashed:false,clickable:false,color:'#0033FF', weight:5} );
	map.addLayer( route );
	var bounds = new L.LatLngBounds( OSRM.G.route.getPositions() );
	map.fitBoundsUI( bounds );
	
	// query better geometry
	var zoom = map.getBoundsZoom(bounds);
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&z='+zoom+'&instructions=false', OSRM.Printing.drawRoute, OSRM.Printing.timeoutRoute, OSRM.DEFAULTS.JSONP_TIMEOUT, 'print');
},
timeoutRoute: function() {},
drawRoute: function(response) {
	if(!response)
		return;
	var geometry = OSRM.RoutingGeometry._decode(response.route_geometry, 5);
	var positions = [];
	for( var i=0, size=geometry.length; i < size; i++)
		positions.push( new L.LatLng(geometry[i][0], geometry[i][1]) );	
	OSRM.Printing.route.setLatLngs( positions );
},



// open printwindow
print: function() {
	// do not open window if there is no route to draw
	if( !OSRM.G.route.isRoute() || !OSRM.G.route.isShown() )
		return;
	// close old window (should we really do this?)
	if( OSRM.printwindow )
		OSRM.printwindow.close();
	OSRM.printwindow = window.open("printing/printing.html","","width=540,height=500,left=100,top=100,dependent=yes,location=no,menubar=no,scrollbars=yes,status=no,toolbar=no,resizable=yes");
	OSRM.printwindow.addEventListener("DOMContentLoaded", OSRM.Printing.printwindowLoaded, false);
},

// add content to printwindow after it has finished loading
printwindowLoaded: function(){
	OSRM.Printing.show( OSRM.G.response );
	OSRM.printwindow.focus();
}

};
