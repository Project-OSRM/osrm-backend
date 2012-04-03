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
		
x: function(){
	OSRM.printwindow.test();
//	var pos1 = OSRM.printwindow.document.getElementById('map1');
//	var pos2 = OSRM.printwindow.document.getElementById('map2');
//	// setup map
//	var osmorgURL = 'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
//	osmorgOptions = {maxZoom: 18};	
//	var osmorg = new L.TileLayer(osmorgURL, osmorgOptions);
//	var temp1 = new OSRM.MapView(pos1, {
//    	center: new L.LatLng(51.505, -0.09),
//	    zoom: 13,
//	    zoomAnimation: false,					// false: removes animations and hiding of routes during zoom
//	    fadeAnimation: false,
//	    layers: [osmorg]
//	});
//	var osmorgURL = 'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
//	osmorgOptions = {maxZoom: 18};	
//	var osmorg = new L.TileLayer(osmorgURL, osmorgOptions);	
//	var temp2 = new OSRM.MapView(pos2, {
//    	center: new L.LatLng(51.505, -0.09),
//	    zoom: 13,
//	    zoomAnimation: false,					// false: removes animations and hiding of routes during zoom
//	    fadeAnimation: false,
//	    layers: [osmorg]
//	});	
},

// react to click
print: function() {
	OSRM.printwindow = window.open("printing.html", "Popupfenster", "width=400,height=300,resizable=yes");
//	fenster.document.write("<div id='map1' style='width:100px;height:100px'>a</div><div id='map2' style='width:100px;height:100px'>b</div>");
	OSRM.printwindow.focus();
	
	OSRM.printwindow.addEventListener("DOMContentLoaded", OSRM.Printing.x, false);
}

};