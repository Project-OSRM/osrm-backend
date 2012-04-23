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

// OSRM initialization
// [for printing window]

OSRM = {};
OSRM.GLOBALS = {};
OSRM.GUI = { visible:false };
OSRM.G = OSRM.GLOBALS;

// function to initialize a map in the new window
function initialize(tile_server) {
 	// setup map
	var tile_layer = new L.TileLayer(tile_server.url, tile_server.options);
	OSRM.G.map = new OSRM.MapView("overview-map", {
    	center: new L.LatLng(48.84, 10.10),
	    zoom: 13,
	    zoomAnimation: false,
	    fadeAnimation: false,
	    layers: [tile_layer],
	    attributionControl: false,
	    zoomControl: false,
	    dragging:false,
	    scrollWheelZoom:false,
	    touchZoom:false,
	    doubleClickZoom:false
	});
	return OSRM.G.map;
}

//print the window
function printWindow() {
	window.print();
}