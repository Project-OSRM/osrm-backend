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

// will hold the Leaflet map object
OSRM.GLOBALS.map = null;


// onload initialization routine
OSRM.init = function() {
	OSRM.prefetchImages();
	OSRM.prefetchIcons();
	
	OSRM.GUI.init();
	OSRM.Map.init();
	OSRM.Routing.init();	
	
 	// check if the URL contains some GET parameter, e.g. for showing a route
 	OSRM.checkURL();
};


// prefetch images
OSRM.GLOBALS.images = Array();
OSRM.prefetchImages = function() {
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
		OSRM.G.images[i] = new Image();
		OSRM.G.images[i].src = images[i];
	}
};


// prefetch icons
OSRM.GLOBALS.icons = Array();
OSRM.prefetchIcons = function() {
	var images = [	'marker-source',
	              	'marker-target',
	              	'marker-via',
	              	'marker-highlight',
	              ];

	for(var i=0; i<images.length; i++)
		OSRM.G.icons[images[i]] = new L.Icon('images/'+images[i]+'.png');
};


// parse URL GET parameters if any exist
OSRM.checkURL = function(){
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
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;
			positions.push ( new L.LatLng( coordinates[0], coordinates[1]) );
		}
		else if(name_val[0] == 'dest') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;				
			destination = new L.LatLng( coordinates[0], coordinates[1]);
		}
		else if(name_val[0] == 'destname') {
			destination_name = decodeURI(name_val[1]).replace(/<\/?[^>]+(>|$)/g ,"");	// discard tags	
		}
		else if(name_val[0] == 'z') {
			zoom = name_val[1];
			if( zoom<0 || zoom > 18)
				return;
		}
		else if(name_val[0] == 'center') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;				
			center = new L.LatLng( coordinates[0], coordinates[1]);			
		}		
	}
		
	// case 1: destination given
	if( destination != undefined ) {
		var index = OSRM.G.markers.setTarget( e.latlng );
		if( destination_name == null )
			OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		else 
			document.getElementById("input-target-name").value = destination_name;
		OSRM.G.markers.route[index].show();
		OSRM.G.markers.route[index].centerView();
		return;
	}

	// case 2: locations given
	if( positions != []) {
		// draw via points
		if( positions.length > 0) {
			OSRM.G.markers.setSource( positions[0] );
			OSRM.Geocoder.updateAddress( OSRM.C.SOURCE_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		}
		if(positions.length > 1) {
			OSRM.G.markers.setTarget( positions[positions.length-1] );
			OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		}
		for(var i=1; i<positions.length-1;i++)
			OSRM.G.markers.setVia( i-1, positions[i] );
		for(var i=0; i<OSRM.G.markers.route.length;i++)
			OSRM.G.markers.route[i].show();
		
		// center on route (support for old links) / move to given view (new behaviour)
		if(zoom == null || center == null) {
			var bounds = new L.LatLngBounds( positions );
			OSRM.g.map.fitBoundsUI( bounds );
		} else {
			OSRM.g.map.setView(center, zoom);
		}
			
		// compute route
		OSRM.Routing.getRoute(OSRM.C.FULL_DESCRIPTION);
	}
};