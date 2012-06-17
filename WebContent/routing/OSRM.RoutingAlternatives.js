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
		
// name of gui buttons to choose alternatives
_buttons: ["gui_a", "gui_b"],
		
// prepare response
prepare: function(response) {
	var original = {};
	original.route_geometry = response.route_geometry;
	original.route_instructions = response.route_instructions;
	original.route_summary = response.route_summary;
	OSRM.G.response.alternatives.push( original );
	
	OSRM.G.response.active_alternative = 0;
	
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
	var buttons = OSRM.RoutingAlternatives._buttons;
	for(var i=0, size=buttons; i<size; i++) {
		if(button_id == i)
			document.getElementById( buttons[i] ).className = "button-pressed top-right-button";
		else
			document.getElementById( buttons[i] ).className = "button top-right-button";
	}
	OSRM.G.response.active_alternative = button_id;	
},

// mouse events on buttons
_click: function(button_id) {
	if( OSRM.G.response.active_alternative == button_id )
		return;
	OSRM.RoutingAlternatives.press(button_id);
	OSRM.G.route.hideAlternativeRoute();
	// switch data
	//OSRM.Routing.showRoute(OSRM.G.response);
	console.log("click "+button_id);	
},
_mouseover: function(button_id) {
	if( OSRM.G.response.active_alternative == button_id )
		return;
	
	var geometry = OSRM.RoutingGeometry._decode(OSRM.G.response.route_geometry, 5);
	OSRM.G.route.showAlternativeRoute(geometry);
	console.log("over "+button_id);	
},
_mouseout: function(button_id) {
	if( OSRM.G.response.active_alternative == button_id )
		return;
	
	OSRM.G.route.hideAlternativeRoute();
	console.log("out "+button_id);	
},

// add alternatives
_addGUI: function() {
	var data =
		'<a class="button top-right-button" id="gui-b">B</a>' +
		'<a class="button top-right-button" id="gui-a">A</a>';
	document.getElementById('information-box-header').innerHTML = data + document.getElementById('information-box-header').innerHTML;
	
	OSRM.RoutingAlternatives.press( OSRM.G.response.active_alternative );
		
	var buttons = OSRM.RoutingAlternatives._buttons;
	for(var i=0, size=buttons; i<size; i++) {
		document.getElementById(buttons[i]).onclick = function () { OSRM.RoutingAlternatives._click(i); };
		document.getElementById(buttons[i]).onmouseover = function () { OSRM.RoutingAlternatives._mouseover(i); };
		document.getElementById(buttons[i]).onmouseout = function () { OSRM.RoutingAlternatives._mouseout(i); };
	}
}

};