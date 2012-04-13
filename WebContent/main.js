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
	var image_list = [	{id:'marker-shadow',					url:L.ROOT_URL + 'images/marker-shadow.png'},
	                  	{id:'marker-source',					url:'images/marker-source.png'},
						{id:'marker-target',					url:'images/marker-target.png'},
						{id:'marker-via',						url:'images/marker-via.png'},
						{id:'marker-highlight',					url:'images/marker-highlight.png'},
						{id:'marker-source-drag',				url:'images/marker-source-drag.png'},
		              	{id:'marker-target-drag',				url:'images/marker-target-drag.png'},
		              	{id:'marker-via-drag',					url:'images/marker-via-drag.png'},
		              	{id:'marker-highlight-drag',			url:'images/marker-highlight-drag.png'},
		              	{id:'marker-drag',						url:'images/marker-drag.png'},
		              	{id:'cancel',							url:'images/cancel.png'},
		              	{id:'cancel_active',					url:'images/cancel_active.png'},
		              	{id:'cancel_hover',						url:'images/cancel_hover.png'},
		              	{id:'restore',							url:'images/restore.png'},
		              	{id:'restore_active',					url:'images/restore_active.png'},
		              	{id:'restore_hover',					url:'images/restore_hover.png'},
		              	{id:'printer',							url:'images/printer.png'},
		              	{id:'printer_active',					url:'images/printer_active.png'},
		              	{id:'printer_hover',					url:'images/printer_hover.png'},
		              	{id:'printer_inactive',					url:'images/printer_inactive.png'},
		        		{id:'direction_0',						url:'images/default.png'},		              	
		              	{id:'direction_1',						url:'images/continue.png'},
		              	{id:'direction_2',						url:'images/slight-right.png'},
		              	{id:'direction_3',						url:'images/turn-right.png'},
		              	{id:'direction_4',						url:'images/sharp-right.png'},
		              	{id:'direction_5',						url:'images/u-turn.png'},
		              	{id:'direction_6',						url:'images/slight-left.png'},
		              	{id:'direction_7',						url:'images/turn-left.png'},
		              	{id:'direction_8',						url:'images/sharp-left.png'},
		        		{id:'direction_11',						url:'images/round-about.png'},
		        		{id:'direction_15',						url:'images/target.png'},
		        		{id:'favicon',							url:'images/osrm-favicon.ico'},
	               ];
		
	for(var i=0; i<image_list.length; i++) {
		OSRM.G.images[image_list[i].id] = new Image();
		OSRM.G.images[image_list[i].id].src = image_list[i].url;
	}
};


// prefetch icons
OSRM.GLOBALS.icons = {};
OSRM.prefetchIcons = function() {
	var icon_list = [	{id:'marker-source',					image_id:'marker-source'},
						{id:'marker-target',					image_id:'marker-target'},
						{id:'marker-via',						image_id:'marker-via'},
						{id:'marker-highlight',					image_id:'marker-highlight'},
						{id:'marker-source-drag',				image_id:'marker-source-drag'},
						{id:'marker-target-drag',				image_id:'marker-target-drag'},
						{id:'marker-via-drag',					image_id:'marker-via-drag'},
						{id:'marker-highlight-drag',			image_id:'marker-highlight-drag'}
						//{id:'marker-drag',						image_id:'marker-drag'}				// special treatment because of size
	              ];

	for(var i=0; i<icon_list.length; i++) {
		var icon = {
				iconUrl: OSRM.G.images[icon_list[i].image_id].src, iconSize: new L.Point(25, 41), iconAnchor: new L.Point(13, 41),
				shadowUrl: OSRM.G.images["marker-shadow"].src, shadowSize: new L.Point(41, 41),
				popupAnchor: new L.Point(0, -33)
			};
		OSRM.G.icons[icon_list[i].id] = new L.SwitchableIcon(icon);
	}
	
	// special values for drag marker
	OSRM.G.icons['marker-drag'] = new L.SwitchableIcon( {iconUrl: OSRM.G.images["marker-drag"].src, iconSize: new L.Point(18, 18) } );	
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
