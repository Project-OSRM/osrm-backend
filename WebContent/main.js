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
// [initialization, image prefetching]


// onload initialization routine
OSRM.init = function() {
	OSRM.prefetchImages();
	OSRM.prefetchIcons();
	
	OSRM.Localization.init();
	OSRM.GUI.init();
	OSRM.Map.init();
	//OSRM.Printing.init();
	OSRM.Routing.init();
	
 	// check if the URL contains some GET parameter, e.g. for showing a route
 	OSRM.parseParameters();
};


// prefetch images
OSRM.GLOBALS.images = {};
OSRM.prefetchImages = function() {
	var image_names = [	'marker-source',
	               	'marker-target',
	              	'marker-via',
	              	'marker-highlight',
	              	'marker-source-drag',
	              	'marker-target-drag',
	              	'marker-via-drag',
	              	'marker-highlight-drag',
	              	'marker-drag',
	              	'cancel',
	              	'cancel_active',
	              	'cancel_hover',
	              	'restore',
	              	'restore_active',
	              	'restore_hover',
	              	'printer',
	              	'printer_active',
	              	'printer_hover',
	              	'printer_inactive',
	        		'turn-left',
	        		'turn-right',
	        		'u-turn',
	        		'continue',
	        		'slight-left',
	        		'slight-right',
	        		'sharp-left',
	        		'sharp-right',
	        		'round-about',
	        		'target',
	        		'default'	        		
	               ];
		
	for(var i=0; i<image_names.length; i++) {
		OSRM.G.images[image_names[i]] = new Image();
		OSRM.G.images[image_names[i]].src = 'images/'+image_names[i]+'.png';
	}

	OSRM.G.images["favicon"] = new Image();
	OSRM.G.images["favicon"].src = 'images/osrm-favicon.ico';
};


// prefetch icons
OSRM.GLOBALS.icons = {};
OSRM.prefetchIcons = function() {
	var image_names = [	'marker-source',
	              	'marker-target',
	              	'marker-via',
	              	'marker-highlight',
	              	'marker-source-drag',
	              	'marker-target-drag',
	              	'marker-via-drag',
	              	'marker-highlight-drag',
	              	'marker-drag'
	              ];

	for(var i=0; i<image_names.length; i++) {
		var icon = {
				iconUrl: 'images/'+image_names[i]+'.png', iconSize: new L.Point(25, 41), iconAnchor: new L.Point(13, 41),
				shadowUrl: L.ROOT_URL + 'images/marker-shadow.png',	shadowSize: new L.Point(41, 41),
				popupAnchor: new L.Point(0, -33)
			};
		OSRM.G.icons[image_names[i]] = new L.SwitchableIcon(icon);
	}
	
	// special values for drag marker
	OSRM.G.icons['marker-drag'] = new L.SwitchableIcon( {iconUrl: 'images/marker-drag.png', iconSize: new L.Point(18, 18) } );	
};


//parse URL GET parameters
OSRM.parseParameters = function(){
	var called_url = document.location.search.substr(1,document.location.search.length);
	
	// reject messages that are clearly too long or too small 
	if( called_url.length > 1000 || called_url.length == 0)
		return;
	
	// storage for parameter values
	var params = {};

	// parse input
	var splitted_url = called_url.split('&');
	for(var i=0; i<splitted_url.length; i++) {
		var name_val = splitted_url[i].split('=');
		if(name_val.length!=2)
			continue;
		
		if(name_val[0] == 'hl') {
			for(var i=0, size=OSRM.Localization.supported_languages.length; i<size; i++) {
				if( OSRM.Localization.supported_languages[i].encoding == name_val[1]) {
					OSRM.Localization.change(name_val[1]);
					break;
				}
			}
		}
		else if(name_val[0] == 'loc') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;
			params.positions = params.positions || [];
			params.positions.push ( new L.LatLng( coordinates[0], coordinates[1]) );
		}
		else if(name_val[0] == 'dest') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;				
			params.destination = new L.LatLng( coordinates[0], coordinates[1]);
		}
		else if(name_val[0] == 'destname') {
			params.destination_name = decodeURI(name_val[1]).replace(/<\/?[^>]+(>|$)/g ,"");	// discard tags	
		}
		else if(name_val[0] == 'z') {
			var zoom_level = Number(name_val[1]);
			if( zoom_level<0 || zoom_level > 18)
				return;
			params.zoom = zoom;
		}
		else if(name_val[0] == 'center') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;				
			params.center = new L.LatLng( coordinates[0], coordinates[1]);			
		}		
	}
		
	// case 1: destination given
	if( params.destination ) {
		var index = OSRM.G.markers.setTarget( params.destination.latlng );
		if( params.destination_name )
			document.getElementById("gui-input-target").value = params.destination_name;
		else 
			OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		OSRM.G.markers.route[index].show();
		OSRM.G.markers.route[index].centerView();
		return;
	}

	// case 2: locations given
	if( params.positions ) {
		// draw via points
		if( params.positions.length > 0 ) {
			OSRM.G.markers.setSource( params.positions[0] );
			OSRM.Geocoder.updateAddress( OSRM.C.SOURCE_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		}
		if( params.positions.length > 1 ) {
			OSRM.G.markers.setTarget( params.positions[params.positions.length-1] );
			OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		}
		for(var i=1; i<params.positions.length-1;i++)
			OSRM.G.markers.setVia( i-1, params.positions[i] );
		for(var i=0; i<OSRM.G.markers.route.length;i++)
			OSRM.G.markers.route[i].show();
		
		// center on route (support for old links) / move to given view (new behaviour)
		if( params.zoom == null || params.center == null ) {
			var bounds = new L.LatLngBounds( params.positions );
			OSRM.G.map.fitBoundsUI( bounds );
		} else {
			OSRM.G.map.setView(params.center, params.zoom);
		}
			
		// compute route
		OSRM.Routing.getRoute();
		return;
	}
	
	// default case: do nothing
};


// onload event
if(document.addEventListener)		// FF, CH
	document.addEventListener("DOMContentLoaded", OSRM.init, false);
else								// old IE
	document.onreadystatechange = function(){if(document.readyState == "interactive" || document.readyState == "complete") OSRM.init();};
