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
OSRM.Control = {};
OSRM.GLOBALS = { main_handle:{boxVisible:function(){return false;}} };	// needed for fitBoundsUI to work
OSRM.GLOBALS.Localization = { culture:"en-US" };						// needed for localized map tiles
OSRM.GLOBALS.DISTANCE_FORMAT = 0;										// needed for scale control
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
			shadowUrl: images_list["marker-shadow"].src,
			iconSize:     [25, 41],
			shadowSize:   [41, 41],
			iconAnchor:   [13, 41],
			shadowAnchor: [13, 41],
			popupAnchor:  [0, -33]
		} });
	for(var i=0; i<icon_list.length; i++) {
		OSRM.G.icons[icon_list[i].id] = new LabelMarkerIcon({iconUrl: images_list[icon_list[i].image_id].src });
	}	
};


// function to initialize a map in the new window
OSRM.drawMap = function(tile_server, bounds) {
 	// setup map
	var tile_layer;
	if( tile_server.bing )	tile_layer = new L.BingLayer(tile_server.apikey, tile_server.options);
	else 					tile_layer = new L.TileLayer(tile_server.url, tile_server.options);
	tile_layer.options.culture = OSRM.G.Localization.culture;
	OSRM.G.map = new OSRM.Control.Map("overview-map", {
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
	
	// add scale control
	OSRM.G.map.scaleControl = new L.Control.Scale();
	OSRM.G.map.scaleControl.options.metric = (OSRM.G.active_distance_format != 1);
	OSRM.G.map.scaleControl.options.imperial = (OSRM.G.active_distance_format == 1);	
	OSRM.G.map.scaleControl.addTo(OSRM.G.map);	
	
	// need to rebuild objects for instanceof to work correctly
    var converted_bounds = new L.LatLngBounds([bounds.getSouthWest().lat, bounds.getSouthWest().lng], [bounds.getNorthEast().lat, bounds.getNorthEast().lng]);
    
	OSRM.G.map.fitBoundsUI(converted_bounds);
	return OSRM.G.map.getBoundsZoom(converted_bounds);
};


// manage makers
OSRM.drawMarkers = function( markers ) {
	// need to rebuild objects for instanceof to work correctly	
	OSRM.G.map.addLayer( new L.LabelMarker( [markers[0].getPosition().lat,markers[0].getPosition().lng] , {draggable:false,clickable:false,icon:OSRM.G.icons['marker-source']} ) );
	var last = markers.length-1;
	for(var i=1; i<last; i++) {
		var via_marker = new L.LabelMarker( [markers[i].getPosition().lat,markers[i].getPosition().lng], {draggable:false,clickable:false,icon:OSRM.G.icons['marker-via']} );
		OSRM.G.map.addLayer( via_marker );
		via_marker.setLabel(i);
	}
	OSRM.G.map.addLayer( new L.LabelMarker( [markers[last].getPosition().lat,markers[last].getPosition().lng], {draggable:false,clickable:false,icon:OSRM.G.icons['marker-target']} ) );
};


// manage route
OSRM.drawRoute = function( positions ) {
	if( OSRM.G.route == undefined )
		OSRM.G.route = new L.Polyline( [] );
	
	// need to rebuild objects for instanceof to work correctly
	var converted_positions = [];	
	if( positions[0].lat )		// geometry returned by OSRM.G.route.getPositions -> original data
		for(var i=0, size=positions.length; i<size; i++)
			converted_positions.push( [positions[i].lat, positions[i].lng] );
	else						// geometry returned by OSRM.RoutingGeometry._decode -> requeried data
		for(var i=0, size=positions.length; i<size; i++)
			converted_positions.push( [positions[i][0], positions[i][1]] );
	
	OSRM.G.route.setLatLngs( converted_positions );
	OSRM.G.route.setStyle( {clickable:false,color:'#0033FF', weight:5, dashArray:""} );
	OSRM.G.map.addLayer( OSRM.G.route );	
};


// start populating the window when it is fully loaded - and only if it was loaded from OSRM 
if(window.opener && window.opener.OSRM)
	window.opener.OSRM.Browser.onLoadHandler( window.opener.OSRM.Printing.printWindowLoaded, window );