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
		
// prepare response
prepare: function() {
	OSRM.RoutingAlternatives._buildAlternatives();
},

// add alternatives
_buildAlternatives: function() {
	var data =
		'<a class="button top-right-button" id="gui-b">B</a>' +
		'<a class="button-pressed top-right-button" id="gui-a">A</a>';
	document.getElementById('information-box-header').innerHTML = data + document.getElementById('information-box-header').innerHTML;
	OSRM.G.response.active = 0;
	
	document.getElementById("gui-a").onclick = function () {
		if( OSRM.G.response.active == 0 )
			return;
		document.getElementById("gui-a").className = "button-pressed top-right-button";
		document.getElementById("gui-b").className = "button top-right-button";
		OSRM.G.route.hideAlternativeRoute();
		OSRM.G.response.active = 0;
		// switch data
		//OSRM.Routing.showRoute(OSRM.G.response);
		console.log("click a");		
	};
	document.getElementById("gui-a").onmouseover = function () {
		if( OSRM.G.response.active == 0 )
			return;
		
		var geometry = OSRM.RoutingGeometry._decode(OSRM.G.response.route_geometry, 5);
		OSRM.G.route.showAlternativeRoute(geometry);
		console.log("over a");
	};
	document.getElementById("gui-a").onmouseout = function () {
		if( OSRM.G.response.active == 0 )
			return;
		
		OSRM.G.route.hideAlternativeRoute();
		console.log("out a");
	};
	
	document.getElementById("gui-b").onclick = function ()  {
		if( OSRM.G.response.active == 1 )
			return;
		document.getElementById("gui-a").className = "button top-right-button";
		document.getElementById("gui-b").className = "button-pressed top-right-button";
		OSRM.G.route.hideAlternativeRoute();
		OSRM.G.response.active = 1;
		// switch data
		//OSRM.Routing.showRoute(OSRM.G.response);
		console.log("click b");
	};
	document.getElementById("gui-b").onmouseover = function () {
		if( OSRM.G.response.active == 1 )
			return;
		
		var geometry = OSRM.RoutingGeometry._decode(OSRM.G.response.route_geometry, 5);
		OSRM.G.route.showAlternativeRoute(geometry);		
		console.log("over b"); 
	};	
	document.getElementById("gui-b").onmouseout = function () {
		if( OSRM.G.response.active == 1 )
			return;
		
		OSRM.G.route.hideAlternativeRoute();
		console.log("out b");
	};	
}

};