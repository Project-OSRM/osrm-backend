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


// map view/model
// [extending Leaflet L.Map with setView/fitBounds methods that respect UI visibility] 
OSRM.MapView = L.Map.extend({
	setViewUI: function(position, zoom) {
		if( OSRM.GUI.visible == true ) {
			var point = OSRM.G.map.project( position, zoom);
			point.x-=OSRM.GUI.width/2;
			position = OSRM.G.map.unproject(point,zoom);		
		}
		this.setView( position, zoom);	
	},
	fitBoundsUI: function(bounds) {
		var southwest = bounds.getSouthWest();
		var northeast = bounds.getNorthEast();
		var zoom = OSRM.G.map.getBoundsZoom(bounds);
		var sw_point = OSRM.G.map.project( southwest, zoom);
		if( OSRM.GUI.visible == true )
			sw_point.x-=OSRM.GUI.width/2;
		else
			sw_point.x-=10;
		sw_point.y+=10;
		var ne_point = OSRM.G.map.project( northeast, zoom);
		ne_point.y-=10;
		sw_point.x+=10;
		bounds.extend( OSRM.G.map.unproject(sw_point,zoom) );
		bounds.extend( OSRM.G.map.unproject(ne_point,zoom) );
		this.fitBounds( bounds );	
	},
	getCenterUI: function(unbounded) {
		var viewHalf = this.getSize();
		if( OSRM.GUI.visible == true )
			viewHalf.x += OSRM.GUI.width;
		var centerPoint = this._getTopLeftPoint().add(viewHalf.divideBy(2));
		
		return this.unproject(centerPoint, this._zoom, unbounded);
	}
});


// map controller
// [map initialization, event handling]
OSRM.Map = {

// map initialization
init: function() {
	// check if GUI is initialized!
	if(OSRM.GUI.visible == null)
		OSRM.GUI.init();
	
	// setup tile servers
	var osmorgURL = 'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
		osmorgAttribution = 'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 Mapnik',
		osmorgOptions = {maxZoom: 18, attribution: osmorgAttribution};

	var osmdeURL = 'http://{s}.tile.openstreetmap.de/tiles/osmde/{z}/{x}/{y}.png',
		osmdeAttribution = 'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 Mapnik',
		osmdeOptions = {maxZoom: 18, attribution: osmdeAttribution};
	
	var mapquestURL = 'http://otile{s}.mqcdn.com/tiles/1.0.0/osm/{z}/{x}/{y}.png',
		mapquestAttribution = 'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 MapQuest',
		mapquestOptions = {maxZoom: 18, attribution: mapquestAttribution, subdomains: '1234'};	
	
	var cloudmadeURL = 'http://{s}.tile.cloudmade.com/BC9A493B41014CAABB98F0471D759707/997/256/{z}/{x}/{y}.png',
    	cloudmadeAttribution = 'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 CloudMade',
	    cloudmadeOptions = {maxZoom: 18, attribution: cloudmadeAttribution};

	var osmorg = new L.TileLayer(osmorgURL, osmorgOptions),
    	osmde = new L.TileLayer(osmdeURL, osmdeOptions),
    	mapquest = new L.TileLayer(mapquestURL, mapquestOptions),
	    cloudmade = new L.TileLayer(cloudmadeURL, cloudmadeOptions);

	// setup map
	OSRM.G.map = new OSRM.MapView('map', {
    	center: new L.LatLng(51.505, -0.09),
	    zoom: 13,
	    zoomAnimation: false,					// false: removes animations and hiding of routes during zoom
	    fadeAnimation: false,
	    layers: [osmorg]
	});

	// add tileservers
	var baseMaps = {
		"osm.org": osmorg,
		"osm.de": osmde,
		"MapQuest": mapquest,
		"CloudMade": cloudmade
	};

	var overlayMaps = {};
	var layersControl = new L.Control.Layers(baseMaps, overlayMaps);
	OSRM.G.map.addControl(layersControl);

    // move zoom markers
	getElementsByClassName(document,'leaflet-control-zoom')[0].style.left=(OSRM.GUI.width+10)+"px";
	getElementsByClassName(document,'leaflet-control-zoom')[0].style.top="5px";

	// initial map position and zoom
	var position = new L.LatLng( OSRM.DEFAULTS.ONLOAD_LATITUDE, OSRM.DEFAULTS.ONLOAD_LONGITUDE);
	OSRM.G.map.setViewUI( position, OSRM.DEFAULTS.ZOOM_LEVEL);

	// map events
	OSRM.G.map.on('zoomend', OSRM.Map.zoomed );
	OSRM.G.map.on('click', OSRM.Map.click );
	OSRM.G.map.on('contextmenu', OSRM.Map.contextmenu );
	OSRM.G.map.on('mousemove', OSRM.Map.mousemove );
},

// map event handlers
zoomed: function(e) { OSRM.Routing.getRoute(); },
contextmenu: function(e) {;},
mousemove: function(e) { OSRM.Via.drawDragMarker(e); },
click: function(e) {
	if( !OSRM.G.markers.hasSource() ) {
		var index = OSRM.G.markers.setSource( e.latlng );
		OSRM.Geocoder.updateAddress( OSRM.C.SOURCE_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		OSRM.G.markers.route[index].show();
		OSRM.G.markers.route[index].centerView( OSRM.G.map.getZoom() );
		OSRM.Routing.getRoute();
	} else if( !OSRM.G.markers.hasTarget() ) {
		var index = OSRM.G.markers.setTarget( e.latlng );
		OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		OSRM.G.markers.route[index].show();
		OSRM.G.markers.route[index].centerView( OSRM.G.map.getZoom() );
		OSRM.Routing.getRoute();
	}	
}
};