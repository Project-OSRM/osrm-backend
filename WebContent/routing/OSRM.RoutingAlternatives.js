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
	{id:"gui-b", label:"B"},
	{id:"gui-c", label:"C"},
	{id:"gui-d", label:"D"}
],


// prepare using alternatives
prepare: function(response) {
	// store basic information
	OSRM.G.alternative_count = response.alternative_geometries.length + 1;
	OSRM.G.alternative_active = 0;
	
	// do nothing if there is no alternative
	if( OSRM.G.alternative_count == 1 )
		return;

	// move best route to alternative array
	console.log("prepare");
	var the_response = OSRM.G.response;
	the_response.alternative_geometries.unshift( response.route_geometry );
	the_response.alternative_instructions.unshift( response.route_instructions );
	the_response.alternative_summaries.unshift( response.route_summary );
	
	// add alternative GUI
	OSRM.RoutingAlternatives._addGUI();
},
prepare_Redraw: function(response) {
	var original = {};
	original.route_geometry = response.route_geometry;
	original.route_instructions = response.route_instructions;
	original.route_summary = response.route_summary;
	OSRM.G.response.alternatives = response.alternatives;
	OSRM.G.response.alternatives.push( original );
	
	OSRM.RoutingAlternatives._addGUI();
},

// press one of the buttons
press: function(button_id) {
	// switch active alternative
	OSRM.G.alternative_active = button_id;
	
	// redraw buttons
	var buttons = OSRM.RoutingAlternatives._buttons;
	for(var i=0, size=OSRM.G.alternative_count; i<size; i++) {
		document.getElementById( buttons[i].id ).className = (button_id == i) ? "button-pressed top-right-button" : "button top-right-button";
	}
},

// mouse events on buttons
_click: function(button_id) {
	console.log("click "+button_id);	
	if( OSRM.G.alternative_active == button_id )
		return;
	OSRM.RoutingAlternatives.press(button_id);
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
	OSRM.RoutingAlternatives._addGUI();
},
_mouseover: function(button_id) {
	console.log("over "+button_id);
	if( OSRM.G.alternative_active == button_id )
		return;

	var geometry = OSRM.RoutingGeometry._decode( OSRM.G.response.alternative_geometries[button_id], 5);
	OSRM.G.route.showAlternativeRoute(geometry);
},
_mouseout: function(button_id) {
	console.log("out "+button_id);
	if( OSRM.G.alternative_active == button_id )
		return;
	
	OSRM.G.route.hideAlternativeRoute();		
},

// add alternatives
_addGUI: function() {
	var buttons = OSRM.RoutingAlternatives._buttons;
	for(var i=0, size=OSRM.G.alternative_count; i<size; i++) {
		var distance = OSRM.Utils.toHumanDistance(OSRM.G.response.alternative_summaries[i].total_distance);
		var time = OSRM.Utils.toHumanTime(OSRM.G.response.alternative_summaries[i].total_time);
		var tooltip = OSRM.loc("DISTANCE")+":"+distance+" "+OSRM.loc("DURATION")+":"+time;
		var data = '<a class="button top-right-button" id="'+buttons[i].id+'" title="'+tooltip+'">'+buttons[i].label+'</a>';
		document.getElementById('information-box-header').innerHTML = data + document.getElementById('information-box-header').innerHTML;
	}
	for(var i=0, size=OSRM.G.alternative_count; i<size; i++) {
		document.getElementById(buttons[i].id).onclick = function (button_id) { return function() {OSRM.RoutingAlternatives._click(button_id); }; }(i) ;
		document.getElementById(buttons[i].id).onmouseover = function (button_id) { return function() {OSRM.RoutingAlternatives._mouseover(button_id); }; } (i);
		document.getElementById(buttons[i].id).onmouseout = function (button_id) { return function() {OSRM.RoutingAlternatives._mouseout(button_id); }; } (i);
	}
	OSRM.RoutingAlternatives.press( OSRM.G.alternative_active );
}

};