var map;


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
	var images = [	'http://map.project-osrm.org/new/images/marker-source.png',
	              	'http://map.project-osrm.org/new/images/marker-target.png',
	              	'http://map.project-osrm.org/new/images/marker-via.png',
	              	'http://map.project-osrm.org/new/images/marker-highlight.png'
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
		OSRM.icons[images[i]] = new L.Icon('http://map.project-osrm.org/new/images/'+images[i]+'.png');
}


// init localization
function initLocale() {
	document.getElementById("gui-route").innerHTML = OSRM.loc("GUI_ROUTE");
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

	map = new L.Map('map', {
    	center: new L.LatLng(51.505, -0.09),
	    zoom: 13,
	    zoomAnimation: false,					// uncomment to remove animations and hiding of routes during zoom
	    fadeAnimation: false,
	    layers: [osmorg]
	});

	var baseMaps = {
		"osm.org": osmorg,
		"osm.de": osmde,
		"MapQuest": mapquest,
		"CloudMade": cloudmade
	};

	var overlayMaps = {};
	var layersControl = new L.Control.Layers(baseMaps, overlayMaps);
    map.addControl(layersControl);

	getElementsByClassName(document,'leaflet-control-zoom')[0].style.left="420px";
	getElementsByClassName(document,'leaflet-control-zoom')[0].style.top="5px";

	map.setView( new L.LatLng( OSRM.DEFAULTS.ONLOAD_LATITUDE, OSRM.DEFAULTS.ONLOAD_LONGITUDE-0.02), OSRM.DEFAULTS.ZOOM_LEVEL);
	map.on('zoomend', function(e) { getRoute(OSRM.FULL_DESCRIPTION); });	

	// onmousemove test	
//	map.on('mousemove', function(e) { console.log("pos: " + e.latlng); });
//	map.on('mousemove', function(e) {
//		var objs = new Array;
//		var obj = null;
//		do {
//			obj = document.elementFromPoint(e.layerPoint.x, e.layerPoint.y);
//			
//			if (obj == null)
//				break;			
//			if (obj == document.body)
//				break;
//			if (obj instanceof SVGPathElement)
//				break;			
//		
//			objs.push(obj);
//			obj.style.display = 'none';
//		} while(true);
//		for(var i=0; i<objs.length; ++i)
//			objs[i].style.display ='';
//		
//		if (obj == null)
//			return;
//		
//		if (obj instanceof SVGPathElement)
//			xroute.route.fire('mousemove',e);
//		else
//			xroute.route.fire('mouseout',e);
//	});
}


// parse URL GET parameters if existing
function checkURL(){
	var called_url = document.location.search.substr(1,document.location.search.length);
	if( called_url != '') {
		var positions = [];

		// parse input (currently only parses start, dest, via)
		var splitted_url = called_url.split('&');
		for(var i=0; i<splitted_url.length; i++) {
			var name_val = splitted_url[i].split('=');
			if(name_val.length!=2)
				continue;
				
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2)
				continue;
				
			if(name_val[0] == 'loc')
				positions.push ( new L.LatLng( coordinates[0], coordinates[1]) );
		}

		// draw via points
		if( positions.length > 0)
			my_markers.setSource( positions[0] );
		for(var i=1; i<positions.length-1;i++)
			my_markers.setVia( i-1, positions[i] );
		if(positions.length > 1)
			my_markers.setTarget( positions[positions.length-1] );
		for(var i=0; i<my_markers.route.length;i++)
			my_markers.route[i].show();
		
		// compute route
		getRoute(OSRM.FULL_DESCRIPTION);
		
		var bounds = new L.LatLngBounds( positions );
		bounds._southWest.lng-=1.02;																// dirty hacks
		map.fitBounds( bounds );
		//my_route.centerView();
	}
}