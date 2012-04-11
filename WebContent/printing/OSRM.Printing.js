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
		
windowLoaded: function(){
	OSRM.printwindow.initialize();
	OSRM.Printing.show( OSRM.G.response );
	OSRM.printwindow.focus();
},

show: function(response) {
	// create route description
	var route_desc = "";
	route_desc += '<table class="results-table">';

	for(var i=0; i < response.route_instructions.length; i++){
		//odd or even ?
		var rowstyle='results-odd';
		if(i%2==0) { rowstyle='results-even'; }

		route_desc += '<tr class="'+rowstyle+'">';
		
		route_desc += '<td class="result-directions">';
		route_desc += '<img width="18px" src="../images/'+OSRM.RoutingDescription.getDirectionIcon(response.route_instructions[i][0])+'" alt="" />';
		route_desc += "</td>";		
		
		route_desc += '<td class="result-items">';
		route_desc += '<span class="result-item">';
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
	headline += OSRM.loc("ROUTE_DESCRIPTION")+":<br/>";
	headline += '<div style="float:left;width:40%">';
	headline += "<span class='route-summary'>"
		+ OSRM.loc("DISTANCE")+": " + OSRM.Utils.metersToDistance(response.route_summary.total_distance)
		+ "<br/>"
		+ OSRM.loc("DURATION")+": " + OSRM.Utils.secondsToTime(response.route_summary.total_time)
		+ "</span>";		
	headline +=	'</div>';

	var output = "";
	output += route_desc;

	OSRM.printwindow.document.getElementById('description-headline').innerHTML = headline;
	OSRM.printwindow.document.getElementById('description').innerHTML = output;
},

// react to click
print: function() {
	OSRM.printwindow = window.open("printing/printing.html","","width=400,height=300,left=100,top=100,dependent=yes,location=no,menubar=no,scrollbars=yes,status=no,toolbar=no,resizable=yes");
	OSRM.printwindow.addEventListener("DOMContentLoaded", OSRM.Printing.windowLoaded, false);
}

};