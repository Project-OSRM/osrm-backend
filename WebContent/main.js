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
	OSRM.prefetchCSSIcons();
	
	OSRM.GUI.init();
	OSRM.Map.init();
	OSRM.Printing.init();
	OSRM.Routing.init();
	OSRM.RoutingAlternatives.init();
	OSRM.Localization.init();	
	
	// stop if in maintenance mode
	if( OSRM.GUI.inMaintenance() == true )
		return;
	
 	// check if the URL contains some GET parameter, e.g. for showing a route
 	OSRM.parseParameters();
 
 	// only init default position / geolocation position if GET parameters do not specify a different one
 	if( OSRM.G.initial_position_override == false )
 		OSRM.Map.initPosition();
 	
 	// finalize initialization of map
 	OSRM.Map.initFinally();
};


// prefetch images
OSRM.GLOBALS.images = {};
OSRM.prefetchImages = function() {
	var image_list = [	{id:'marker-shadow',					url:'leaflet/images/marker-shadow.png'},
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
		              	{id:'up',								url:'images/up.png'},
		              	{id:'up_active',						url:'images/up_active.png'},
		              	{id:'up_hover',							url:'images/up_hover.png'},		
		              	{id:'down',								url:'images/down.png'},
		              	{id:'down_active',						url:'images/down_active.png'},
		              	{id:'down_hover',						url:'images/down_hover.png'},
		              	{id:'config',							url:'images/config.png'},
		              	{id:'config_active',					url:'images/config_active.png'},
		              	{id:'config_hover',						url:'images/config_hover.png'},		              	
		              	{id:'mapping',							url:'images/mapping.png'},
		              	{id:'mapping_active',					url:'images/mapping_active.png'},
		              	{id:'mapping_hover',					url:'images/mapping_hover.png'},		              	
		              	{id:'printer',							url:'images/printer.png'},
		              	{id:'printer_active',					url:'images/printer_active.png'},
		              	{id:'printer_hover',					url:'images/printer_hover.png'},
		              	{id:'printer_inactive',					url:'images/printer_inactive.png'},
		              	{id:'zoom_in',							url:'images/zoom_in.png'},
		              	{id:'zoom_in_active',					url:'images/zoom_in_active.png'},
		              	{id:'zoom_in_hover',					url:'images/zoom_in_hover.png'},
		              	{id:'zoom_out',							url:'images/zoom_out.png'},
		              	{id:'zoom_out_active',					url:'images/zoom_out_active.png'},
		              	{id:'zoom_out_hover',					url:'images/zoom_out_hover.png'},		              	
		              	{id:'locations_user',					url:'images/locations_user.png'},
		              	{id:'locations_user_active',			url:'images/locations_user_active.png'},
		              	{id:'locations_user_hover',				url:'images/locations_user_hover.png'},
		              	{id:'locations_user_inactive',			url:'images/locations_user_inactive.png'},
		              	{id:'locations_route',					url:'images/locations_route.png'},
		              	{id:'locations_route_active',			url:'images/locations_route_active.png'},
		              	{id:'locations_route_hover',			url:'images/locations_route_hover.png'},
		              	{id:'locations_route_inactive',			url:'images/locations_route_inactive.png'},
		              	{id:'layers',							url:'images/layers.png'},
		        		{id:'direction_0',						url:'images/default.png'},		              	
		              	{id:'direction_1',						url:'images/continue.png'},
		              	{id:'direction_2',						url:'images/slight-right.png'},
		              	{id:'direction_3',						url:'images/turn-right.png'},
		              	{id:'direction_4',						url:'images/sharp-right.png'},
		              	{id:'direction_5',						url:'images/u-turn.png'},
		              	{id:'direction_6',						url:'images/sharp-left.png'},
		              	{id:'direction_7',						url:'images/turn-left.png'},
		              	{id:'direction_8',						url:'images/slight-left.png'},
		              	{id:'direction_10',						url:'images/head.png'},
		        		{id:'direction_11',						url:'images/round-about.png'},
		        		{id:'direction_15',						url:'images/target.png'},
		        		{id:'osrm-logo',						url:'images/osrm-logo.png'},
		        		{id:'selector',							url:'images/selector.png'}
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
	
	// special values for drag marker
	OSRM.G.icons['marker-drag'] = new L.LabelMarkerIcon( {iconUrl: OSRM.G.images["marker-drag"].getAttribute("src"), iconSize: new L.Point(18, 18) } );
};


// set css styles for images
OSRM.prefetchCSSIcons = function() {
	var css_list = [
	                	{ id:'#gui-printer-inactive',			image_id:'printer_inactive'},
	                	{ id:'#gui-printer',					image_id:'printer'},
	                	{ id:'#gui-printer:hover',				image_id:'printer_hover'},
	                	{ id:'#gui-printer:active',				image_id:'printer_active'},
                	
	                	{ id:'.gui-zoom-in',					image_id:'zoom_in'},
	                	{ id:'.gui-zoom-in:hover',				image_id:'zoom_in_hover'},
	                	{ id:'.gui-zoom-in:active',				image_id:'zoom_in_active'},
	                	
	                	{ id:'.gui-zoom-out',					image_id:'zoom_out'},
	                	{ id:'.gui-zoom-out:hover',				image_id:'zoom_out_hover'},
	                	{ id:'.gui-zoom-out:active',			image_id:'zoom_out_active'},
	                	
	                	{ id:'.gui-locations-user-inactive',	image_id:'locations_user_inactive'},
	                	{ id:'.gui-locations-user',				image_id:'locations_user'},
	                	{ id:'.gui-locations-user:hover',		image_id:'locations_user_hover'},
	                	{ id:'.gui-locations-user:active',		image_id:'locations_user_active'},
	                	
	                	{ id:'.gui-locations-route-inactive',	image_id:'locations_route_inactive'},
	                	{ id:'.gui-locations-route',			image_id:'locations_route'},
	                	{ id:'.gui-locations-route:hover',		image_id:'locations_route_hover'},
	                	{ id:'.gui-locations-route:active',		image_id:'locations_route_active'},
	                	
	                	{ id:'.gui-layers',						image_id:'layers'},
	                	
	                	{ id:'.cancel-marker',					image_id:'cancel'},
	                	{ id:'.cancel-marker:hover',			image_id:'cancel_hover'},
	                	{ id:'.cancel-marker:active',			image_id:'cancel_active'},
	                	
	                	{ id:'.up-marker',						image_id:'up'},
	                	{ id:'.up-marker:hover',				image_id:'up_hover'},
	                	{ id:'.up-marker:active',				image_id:'up_active'},
	                	
	                	{ id:'.down-marker',					image_id:'down'},
	                	{ id:'.down-marker:hover',				image_id:'down_hover'},
	                	{ id:'.down-marker:active',				image_id:'down_active'},
	                	
	                	{ id:'#input-mask-header',				image_id:'osrm-logo'},
	                	{ id:'.styled-select',					image_id:'selector'},
	                	
	                	{ id:'#config-handle-icon',				image_id:'config'},
	                	{ id:'#config-handle-icon:hover',		image_id:'config_hover'},
	                	{ id:'#config-handle-icon:active',		image_id:'config_active'},
	                	           	
	                	{ id:'#mapping-handle-icon',			image_id:'mapping'},
	                	{ id:'#mapping-handle-icon:hover',		image_id:'mapping_hover'},
	                	{ id:'#mapping-handle-icon:active',		image_id:'mapping_active'},
	                	          	
	                	{ id:'#main-handle-icon',				image_id:'restore'},
	                	{ id:'#main-handle-icon:hover',			image_id:'restore_hover'},
	                	{ id:'#main-handle-icon:active',		image_id:'restore_active'}	                	
	                ];
	
	var stylesheet = OSRM.CSS.getStylesheet("main.css");
	for(var i=0; i<css_list.length; i++) {
		OSRM.CSS.insert( stylesheet, css_list[i].id, "background-image:url("+ OSRM.G.images[css_list[i].image_id].getAttribute("src") + ");" );
	}
};


//parse URL GET parameters
OSRM.parseParameters = function(){
	var called_url = document.location.search.substr(1,document.location.search.length);
	
	// state, if GET parameters specify a different initial position
	OSRM.G.initial_position_override = false;
	
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
			OSRM.Localization.setLanguage(name_val[1]);
		}
		else if(name_val[0] == 'df') {
			var type = parseInt(name_val[1]);
			if(type != 0 && type != 1)
				return;
			OSRM.GUI.setDistanceFormat(type);
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
			params.destination = new L.LatLng( coordinates[0], coordinates[1] );
		}
		else if(name_val[0] == 'destname') {
			params.destination_name = decodeURI(name_val[1]).replace(/<\/?[^>]+(>|$)/g ,"");	// discard tags	
		}
		else if(name_val[0] == 'z') {
			var zoom_level = Number(name_val[1]);
			if( zoom_level<0 || zoom_level > 18)
				return;
			params.zoom = zoom_level;
		}
		else if(name_val[0] == 'center') {
			var coordinates = unescape(name_val[1]).split(',');
			if(coordinates.length!=2 || !OSRM.Utils.isLatitude(coordinates[0]) || !OSRM.Utils.isLongitude(coordinates[1]) )
				return;				
			params.center = new L.LatLng( coordinates[0], coordinates[1]);			
		}
		else if(name_val[0] == 'alt') {
			var active_alternative = Number(name_val[1]);
			if( active_alternative<0 || active_alternative>OSRM.RoutingAlternatives>10)	// using 10 as arbitrary upper limit
				return;
			params.active_alternative = active_alternative;
		}
		else if(name_val[0] == 're') {
			var active_routing_engine = Number(name_val[1]);
			if( active_routing_engine<0 || active_routing_engine>=OSRM.DEFAULTS.ROUTING_ENGINES.length)
				return;
			params.active_routing_engine = active_routing_engine;
		}
	}
		
	// case 1: destination given
	if( params.destination ) {
		var index = OSRM.G.markers.setTarget( params.destination );
		if( params.destination_name )
			document.getElementById("gui-input-target").value = params.destination_name;
		else 
			OSRM.Geocoder.updateAddress( OSRM.C.TARGET_LABEL, OSRM.C.DO_FALLBACK_TO_LAT_LNG );
		OSRM.G.markers.route[index].show();
		OSRM.G.markers.route[index].centerView();
		OSRM.G.initial_position_override = true;
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
		
		// set active alternative (if via points are set or alternative does not exists: automatic fallback to shortest route)
		OSRM.G.active_alternative = params.active_alternative || 0;
		
		// set routing server
		OSRM.GUI.setRoutingEngine( params.active_routing_engine || OSRM.DEFAULTS.ROUTING_ENGINE );
			
		// compute route
		OSRM.Routing.getRoute({keepAlternative:true});
		OSRM.G.initial_position_override = true;
		return;
	}
	
	// default case: do nothing	
	return;
};


// onload event
OSRM.Browser.onLoadHandler( OSRM.init );
