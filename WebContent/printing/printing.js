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
OSRM.GLOBALS = { main_handle:{boxVisible:function(){return false;}} };	// needed for fitBoundsUI to work
OSRM.Localization = { current_language:"en"};							// needed for localized map tiles
OSRM.G = OSRM.GLOBALS;


//print the window
function printWindow() {
	window.print();
}


//prefetch icons
OSRM.GLOBALS.icons = {};
OSRM.prefetchIcons = function(images_list) {
	var icon_list = [	{id:'marker-source',					image_id:'marker-source'},
						{id:'marker-target',					image_id:'marker-target'},
						{id:'marker-via',						image_id:'marker-via'},
						{id:'marker-highlight',					image_id:'marker-highlight'}
		              ];
	
	var LabelMarkerIcon = L.LabelMarkerIcon.extend({
		options: {
			shadowUrl: OSRM.G.images["marker-shadow"].getAttribute("src"),
			iconSize:     [25, 41],
			shadowSize:   [41, 41],
			iconAnchor:   [13, 41],
			shadowAnchor: [13, 41],
			popupAnchor:  [0, -33]
		} });
	for(var i=0; i<icon_list.length; i++) {
		OSRM.G.icons[icon_list[i].id] = new LabelMarkerIcon({iconUrl: OSRM.G.images[icon_list[i].image_id].getAttribute("src")});
	}	
};


// function to initialize a map in the new window
OSRM.drawMap = function(tile_server, bounds) {
 	// setup map
	var tile_layer;
	if( tile_server.bing )	tile_layer = new L.BingLayer(tile_server.apikey, tile_server.options);
	else 					tile_layer = new L.TileLayer(tile_server.url, tile_server.options);
	tile_layer.options.culture = OSRM.loc("CULTURE");
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
	
	OSRM.G.map.fitBoundsUI(bounds);
	return OSRM.G.map.getBoundsZoom(bounds);
};


// manage makers
OSRM.drawMarkers = function( markers ) {
	OSRM.G.map.addLayer( new L.LabelMarker( markers[0].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-source']} ) );
	for(var i=1, size=markers.length-1; i<size; i++) {
		var via_marker = new L.LabelMarker( markers[i].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-via']} );
		OSRM.G.map.addLayer( via_marker );
		via_marker.setLabel(i);
	}
	OSRM.G.map.addLayer( new L.LabelMarker( markers[markers.length-1].getPosition(), {draggable:false,clickable:false,icon:OSRM.G.icons['marker-target']} ) );
};


// manage route
OSRM.drawRoute = function( positions ) {
	if( OSRM.G.route == undefined )
		OSRM.G.route = new L.Polyline();
	OSRM.G.route.setLatLngs( positions );
	OSRM.G.route.setStyle( {clickable:false,color:'#0033FF', weight:5, dashArray:""} );
	OSRM.G.map.addLayer( OSRM.G.route );	
};
OSRM.updateRoute = function( positions ) {
	OSRM.G.route.setLatLngs( positions );
};


// start populating the window when it is fully loaded - and only if it was loaded from OSRM 
if(window.opener && window.opener.OSRM)
	window.opener.OSRM.Browser.onLoadHandler( window.opener.OSRM.Printing.printWindowLoaded, window );