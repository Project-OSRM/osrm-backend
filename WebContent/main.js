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
// [initialization of maps, local strings, image prefetching]

var map;


// onload initialization routine
function init() {
	prefetchImages();
	prefetchIcons();
	
	initLocale();
	initMap();
	initRouting();

 	// check if the URL contains some GET parameter, e.g. for the route
 	checkURL();	
}


// prefetch images
OSRM.images = Array();
function prefetchImages() {
	var images = [	'images/marker-source.png',
	              	'images/marker-target.png',
	              	'images/marker-via.png',
	              	'images/marker-highlight.png',
	              	'images/cancel.png',
	              	'images/cancel_active.png',
	              	'images/cancel_hover.png',
	              	'images/restore.png',
	              	'images/restore_active.png',
	              	'images/restore_hover.png'
	              ];

	for(var i=0; i<images.length; i++) {
		OSRM.images[i] = new Image();
		OSRM.images[i].src = images[i];
	}
}


// prefetch icons
OSRM.icons = Array();
function prefetchIcons() {
	var images = [	'marker-source',
	              	'marker-target',
	              	'marker-via',
	              	'marker-highlight',
	              ];

	for(var i=0; i<images.length; i++)
		OSRM.icons[images[i]] = new L.Icon('images/'+images[i]+'.png');
}


// init localization
function initLocale() {
	document.getElementById("gui-reset").innerHTML = OSRM.loc("GUI_RESET");
	document.getElementById("gui-reverse").innerHTML = OSRM.loc("GUI_REVERSE");
	document.getElementById("gui-option-highlight-nonames-label").innerHTML = OSRM.loc("GUI_HIGHLIGHT_UNNAMED_ROADS");
	document.getElementById("options-toggle").innerHTML = OSRM.loc("GUI_OPTIONS");
	document.getElementById("gui-search-source").innerHTML = OSRM.loc("GUI_SEARCH");
	document.getElementById("gui-search-target").innerHTML = OSRM.loc("GUI_SEARCH");
	document.getElementById("gui-search-source-label").innerHTML = OSRM.loc("GUI_START")+":";
	document.getElementById("gui-search-target-label").innerHTML = OSRM.loc("GUI_END")+":";
	document.getElementById("input-source-name").title = OSRM.loc("GUI_START_TOOLTIP");
	document.getElementById("input-target-name").title = OSRM.loc("GUI_END_TOOLTIP");
	document.getElementById("legal-notice").innerHTML = OSRM.loc("GUI_LEGAL_NOTICE");
	
	document.getElementById('input-source-name').value = OSRM.DEFAULTS.ONLOAD_SOURCE;
	document.getElementById('input-target-name').value = OSRM.DEFAULTS.ONLOAD_TARGET;
}


// centering on geolocation
function callbak_centerOnGeolocation( position ) {
	map.setView( new L.LatLng( position.coords.latitude, position.coords.longitude-0.02), OSRM.DEFAULTS.ZOOM_LEVEL);
}
function centerOnGeolocation() {
	if (navigator.geolocation)
		navigator.geolocation.getCurrentPosition( callbak_centerOnGeolocation );
}


// init map
function initMap() {
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
	map = new L.Map('map', {
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
    map.addControl(layersControl);

    // move zoom markers
	getElementsByClassName(document,'leaflet-control-zoom')[0].style.left="420px";
	getElementsByClassName(document,'leaflet-control-zoom')[0].style.top="5px";

	// initial map position and zoom
	map.setView( new L.LatLng( OSRM.DEFAULTS.ONLOAD_LATITUDE, OSRM.DEFAULTS.ONLOAD_LONGITUDE), OSRM.DEFAULTS.ZOOM_LEVEL);
	map.on('zoomend', function(e) { getRoute(OSRM.FULL_DESCRIPTION); });
	map.on('contextmenu', function(e) {});

	// click on map to set source and target nodes
	map.on('click', function(e) {
		if( !my_markers.hasSource() )
			onclickGeocoderResult("source", e.latlng.lat, e.latlng.lng, true, false );
		else if( !my_markers.hasTarget() )
			onclickGeocoderResult("target", e.latlng.lat, e.latlng.lng, true, false );
	} );
}


// parse URL GET parameters if any exist
function checkURL(){
	var called_url = document.location.search.substr(1,document.location.search.length);
	
	// reject messages that are clearly too long or too small 
	if( called_url.length > 1000 || called_url.length == 0)
		return;
	
	// storage for parameter values
	var positions = [];
	var zoom = null;
	var center = null;
	var destination = null;
	var destination_name = null;

	// parse input
	var splitted_url = called_url.split('&');
	for(var i=0; i<splitted_url.length; i++) {
		var name_val = splitted_url[i].split('=');
		if(name_val.length!=2)
			continue;
			
		if(name_val[0] == 'loc') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !isLatitude(coordinates[0]) || !isLongitude(coordinates[1]) )
				return;
			positions.push ( new L.LatLng( coordinates[0], coordinates[1]) );
		}
		else if(name_val[0] == 'dest') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !isLatitude(coordinates[0]) || !isLongitude(coordinates[1]) )
				return;				
			destination = new L.LatLng( coordinates[0], coordinates[1]);
		}
		else if(name_val[0] == 'destname') {
			destination_name = decodeURI(name_val[1]).replace(/<\/?[^>]+(>|$)/g ,"");	// discard tags	
		}
		else if(name_val[0] == 'zoom') {
			zoom = name_val[1];
			if( zoom<0 || zoom > 18)
				return;
		}
		else if(name_val[0] == 'center') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !isLatitude(coordinates[0]) || !isLongitude(coordinates[1]) )
				return;				
			center = new L.LatLng( coordinates[0], coordinates[1]);			
		}		
	}
		
	// case 1: destination given
	if( destination != undefined ) {
		onclickGeocoderResult("target", destination.lat, destination.lng, (destination_name == undefined) );
		if( destination_name != undefined )
			document.getElementById("input-target-name").value = destination_name;
		return;
	}

	// case 2: locations given
	if( positions != []) {
		// draw via points
		if( positions.length > 0) {
			onclickGeocoderResult("source", positions[0].lat, positions[0].lng, true, false );
			//my_markers.setSource( positions[0] );
		}
		if(positions.length > 1) {
			onclickGeocoderResult("target", positions[positions.length-1].lat, positions[positions.length-1].lng, true, false );
			//my_markers.setTarget( positions[positions.length-1] );			
		}
		for(var i=1; i<positions.length-1;i++)
			my_markers.setVia( i-1, positions[i] );
		for(var i=0; i<my_markers.route.length;i++)
			my_markers.route[i].show();
		
		// center on route (support for old links) / move to given view (new behaviour)
		if(zoom == null || center == null) {
			var bounds = new L.LatLngBounds( positions );
			map.fitBounds( bounds );
		} else {
			map.setView(center, zoom);
		}
			
		// compute route
		getRoute(OSRM.FULL_DESCRIPTION);
	}
}
