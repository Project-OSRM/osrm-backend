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

// OSRM map handling
// [initialization, event handling, centering relative to UI]

// will hold the map object
OSRM.GLOBALS.map = null;
OSRM.GLOBALS.localizable_maps = [];


// map controller
// [map initialization, event handling]
OSRM.Map = {

// map initialization
init: function() {
	// check if GUI is initialized!
	if(OSRM.G.main_handle == null)
		OSRM.GUI.init();
	
	// setup tile servers
	var tile_servers = OSRM.DEFAULTS.TILE_SERVERS;
	var base_maps = {};
	for(var i=0, size=tile_servers.length; i<size; i++) {
		if( tile_servers[i].bing == true ) {
			base_maps[ tile_servers[i].display_name ] = new L.BingLayer( tile_servers[i].apikey, tile_servers[i].options );
			OSRM.G.localizable_maps.push( base_maps[ tile_servers[i].display_name ] );
		} else {
			tile_servers[i].options.attribution = tile_servers[i].attribution; 
			base_maps[ tile_servers[i].display_name ] = new L.TileLayer( tile_servers[i].url, tile_servers[i].options );
		}
		L.Util.stamp( base_maps[ tile_servers[i].display_name ] );			// stamp tile servers so that their order is correct in layers control
	}

	// setup map
	OSRM.G.map = new OSRM.Control.Map('map', {
    	center: new L.LatLng(OSRM.DEFAULTS.ONLOAD_LATITUDE, OSRM.DEFAULTS.ONLOAD_LONGITUDE),
	    zoom: OSRM.DEFAULTS.ONLOAD_ZOOM_LEVEL,
	    layers: [base_maps[tile_servers[0].display_name]],	    
	    zoomAnimation: false,								// animations have to be inactive during initialization (leaflet issue #918)
	    fadeAnimation: false,
	    zoomControl: false									// use OSRM zoom buttons
	});

	// add locations control
	OSRM.G.map.locationsControl = new OSRM.Control.Locations();
	OSRM.G.map.locationsControl.addTo(OSRM.G.map);
	
	// add layer control
	OSRM.G.map.layerControl = new OSRM.Control.Layers(base_maps, {});
	OSRM.G.map.layerControl.addTo(OSRM.G.map);	

	// add zoom control
	OSRM.G.map.zoomControl = new OSRM.Control.Zoom();
	OSRM.G.map.zoomControl.addTo(OSRM.G.map);
	OSRM.G.map.zoomControl.show();
	
	// add scale control
	OSRM.G.map.scaleControl = new L.Control.Scale();
	OSRM.G.map.scaleControl.options.metric = (OSRM.G.DISTANCE_FORMAT != 1);
	OSRM.G.map.scaleControl.options.imperial = (OSRM.G.DISTANCE_FORMAT == 1);	
	OSRM.G.map.scaleControl.addTo(OSRM.G.map);

	// map events
	OSRM.G.map.on('zoomend', OSRM.Map.zoomed );
	OSRM.G.map.on('click', OSRM.Map.click );
	OSRM.G.map.on('contextmenu', OSRM.Map.contextmenu );
	OSRM.G.map.on('mousemove', OSRM.Map.mousemove );
},
initFinally: function() {
	L.Util.setOptions( OSRM.G.map, {
		zoomAnimation: true,
		fadeAnimation: true
	} );
},

// init map position and zoom (respect UI visibility / use browser geolocation) 
initPosition: function() {
	var position = new L.LatLng( OSRM.DEFAULTS.ONLOAD_LATITUDE, OSRM.DEFAULTS.ONLOAD_LONGITUDE);
	OSRM.G.map.setViewUI( position, OSRM.DEFAULTS.ONLOAD_ZOOM_LEVEL, true);
	if (navigator.geolocation && document.URL.indexOf("file://") == -1)		// convenience: FF does not save access rights for local files 
		navigator.geolocation.getCurrentPosition(OSRM.Map.geolocationResponse);
},

// map event handlers
zoomed: function(e) {
	if(OSRM.G.dragging)
		OSRM.Routing.getRoute_Dragging();
	else
		OSRM.Routing.getRoute_Redraw({keepAlternative:true});
},
contextmenu: function(e) {;},
mousemove: function(e) { OSRM.Via.drawDragMarker(e); },
click: function(e) {
	OSRM.GUI.deactivateTooltip( "CLICKING" );	
	if( !OSRM.G.markers.hasSource() ) {
		var index = OSRM.G.markers.setSource( e.latlng );
		OSRM.Geocoder.updateAddress( OSRM.C.SOURCE_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		OSRM.G.markers.route[index].show();		
		OSRM.Routing.getRoute( {recenter:OSRM.G.markers.route.length == 2} );	// allow recentering when the route is first shown 
	} else if( !OSRM.G.markers.hasTarget() ) {
		var index = OSRM.G.markers.setTarget( e.latlng );
		OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		OSRM.G.markers.route[index].show();
		OSRM.Routing.getRoute( {recenter:OSRM.G.markers.route.length == 2} );	// allow recentering when the route is first shown
	}
},
geolocationResponse: function(response) {
	var latlng = new L.LatLng(response.coords.latitude, response.coords.longitude);		
	OSRM.G.map.setViewUI(latlng, OSRM.DEFAULTS.ZOOM_LEVEL );
}
};
