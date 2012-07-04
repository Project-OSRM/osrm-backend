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

// OSRM routing alternatives
// [everything about handling alternatives]


OSRM.RoutingAlternatives = {
		
// remaining problems: how to handle PRINTING (do it internally), LINKS (add parameter to JSONP call)
		
// data of gui buttons for alternativess
_buttons: [ 
	{id:"gui-a", label:"A"},
	{id:"gui-b", label:"B"}
],


// reset stored values
reset: function() {
	OSRM.G.active_alternative = 0;
	OSRM.G.alternative_count = 0;
},

// restructure response data  
init: function(response) {
	// move best route to alternative array
	var the_response = OSRM.G.response;
	the_response.alternative_geometries.unshift( response.route_geometry );
	the_response.alternative_instructions.unshift( response.route_instructions );
	the_response.alternative_summaries.unshift( response.route_summary );
	
	// update basic information
	OSRM.G.alternative_count = response.alternative_geometries.length;
	if( OSRM.G.active_alternative == undefined )
		OSRM.G.active_alternative = 0;
	if( OSRM.G.active_alternative > OSRM.G.alternative_count - 1 )
		OSRM.G.active_alternative = 0;
	
	// switch data
	the_response.route_geometry = the_response.alternative_geometries[OSRM.G.active_alternative];
	the_response.route_instructions = the_response.alternative_instructions[OSRM.G.active_alternative];
	the_response.route_summary = the_response.alternative_summaries[OSRM.G.active_alternative];	
},

// prepare using alternatives
prepare: function(response) {
	// store basic information
	OSRM.G.alternative_count = response.alternative_geometries.length + 1;
	OSRM.G.active_alternative = 0;
	
	// do nothing if there is no alternative
//	if( OSRM.G.alternative_count == 1 )
//		return;

	// move best route to alternative array
	var the_response = OSRM.G.response;
	the_response.alternative_geometries.unshift( response.route_geometry );
	the_response.alternative_instructions.unshift( response.route_instructions );
	the_response.alternative_summaries.unshift( response.route_summary );
	
	// add alternative GUI
	OSRM.RoutingAlternatives._drawGUI();
},
prepare_Redraw: function(response) {
	// do nothing if there is no alternative
	if( OSRM.G.alternative_count == 1 )
		return;	
	
	// move results
	var the_response = OSRM.G.response;
	the_response.alternative_geometries = response.alternative_geometries;
	the_response.alternative_instructions = response.alternative_instructions;
	the_response.alternative_summaries = response.alternative_summaries;
	
	the_response.alternative_geometries.unshift( response.route_geometry );
	the_response.alternative_instructions.unshift( response.route_instructions );
	the_response.alternative_summaries.unshift( response.route_summary );	

	// switch data
	the_response.route_geometry = the_response.alternative_geometries[OSRM.G.active_alternative];
	the_response.route_instructions = the_response.alternative_instructions[OSRM.G.active_alternative];
	the_response.route_summary = the_response.alternative_summaries[OSRM.G.active_alternative];
},

// switch active alternative and redraw buttons accordingly
setActive: function(button_id) {
	// switch active alternative
	OSRM.G.active_alternative = button_id;
	
	// redraw clickable buttons
	var buttons = OSRM.RoutingAlternatives._buttons;
	for(var i=0, size=OSRM.G.alternative_count; i<size; i++) {
		document.getElementById( buttons[i].id ).className = (button_id == i) ? "button-pressed top-right-button" : "button top-right-button";
	}
	// do nothing for non-clickable buttons
},

// mouse events on buttons
_click: function(button_id) {
	if( OSRM.G.active_alternative == button_id )
		return;
	OSRM.RoutingAlternatives.setActive(button_id);
	OSRM.G.route.hideAlternativeRoute();
	
	// switch data
	var the_response = OSRM.G.response;
	the_response.route_geometry = the_response.alternative_geometries[button_id];
	the_response.route_instructions = the_response.alternative_instructions[button_id];
	the_response.route_summary = the_response.alternative_summaries[button_id];
	
	// show alternative
	OSRM.RoutingGeometry.show(the_response);
	OSRM.RoutingNoNames.show(the_response);
	OSRM.RoutingDescription.show(the_response);
	OSRM.RoutingAlternatives._drawGUI();
},
_mouseover: function(button_id) {
	if( OSRM.G.active_alternative == button_id )
		return;

	var geometry = OSRM.RoutingGeometry._decode( OSRM.G.response.alternative_geometries[button_id], 5);
	OSRM.G.route.showAlternativeRoute(geometry);
},
_mouseout: function(button_id) {
	if( OSRM.G.active_alternative == button_id )
		return;
	
	OSRM.G.route.hideAlternativeRoute();		
},

// draw alternative GUI
_drawGUI: function() {
	var buttons = OSRM.RoutingAlternatives._buttons;
	// draw clickable buttons
	for(var i=0, size=OSRM.G.alternative_count; i<size; i++) {
		var distance = OSRM.Utils.toHumanDistance(OSRM.G.response.alternative_summaries[i].total_distance);
		var time = OSRM.Utils.toHumanTime(OSRM.G.response.alternative_summaries[i].total_time);
		var tooltip = OSRM.loc("DISTANCE")+":"+distance+" "+OSRM.loc("DURATION")+":"+time;
		var buttonClass = (i == OSRM.G.active_alternative) ? "button-pressed" : "button";
		var data = '<a class="'+buttonClass+' top-right-button" id="'+buttons[i].id+'" title="'+tooltip+'">'+buttons[i].label+'</a>';
		document.getElementById('information-box-header').innerHTML = data + document.getElementById('information-box-header').innerHTML;
	}
	// draw non-clickable buttons
	for(var i=OSRM.G.alternative_count, size=buttons.length; i<size; ++i) {
		var data = '<a class="button-inactive top-right-button" id="'+buttons[i].id+'">'+buttons[i].label+'</a>';
		document.getElementById('information-box-header').innerHTML = data + document.getElementById('information-box-header').innerHTML;		
	}
	// add events
	for(var i=0, size=OSRM.G.alternative_count; i<size; i++) {
		document.getElementById(buttons[i].id).onclick = function (button_id) { return function() {OSRM.RoutingAlternatives._click(button_id); }; }(i) ;
		document.getElementById(buttons[i].id).onmouseover = function (button_id) { return function() {OSRM.RoutingAlternatives._mouseover(button_id); }; } (i);
		document.getElementById(buttons[i].id).onmouseout = function (button_id) { return function() {OSRM.RoutingAlternatives._mouseout(button_id); }; } (i);
	}
}

};