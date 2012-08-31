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

// OSRM config file
// [has to be loaded directly after OSRM.base]

OSRM.DEFAULTS = {
	ROUTING_ENGINES: [
		{	url: 'http://router.project-osrm.org/viaroute',
			timestamp: 'http://router.project-osrm.org/timestamp',
			metric: 0,
			label: 'ENGINE_0',
		}
	],
	
	WEBSITE_URL: document.URL.replace(/#*(\?.*|$)/i,""),					// truncates URL before first ?, and removes tailing #
	HOST_GEOCODER_URL: 'http://nominatim.openstreetmap.org/search',
	HOST_REVERSE_GEOCODER_URL: 'http://nominatim.openstreetmap.org/reverse',
	HOST_SHORTENER_URL: 'http://map.project-osrm.org/shorten/',
	
	SHORTENER_PARAMETERS: '%url&jsonp=%jsonp',
	SHORTENER_REPLY_PARAMETER: 'ShortURL',	
	
	ROUTING_ENGINE: 0,
	DISTANCE_FORMAT: 0,														// 0: km, 1: miles
	GEOCODER_BOUNDS: '',	
	ZOOM_LEVEL: 14,
	HIGHLIGHT_ZOOM_LEVEL: 16,
	JSONP_TIMEOUT: 10000,
	
	ONLOAD_ZOOM_LEVEL: 5,
	ONLOAD_LATITUDE: 48.84,
	ONLOAD_LONGITUDE: 10.10,
	ONLOAD_SOURCE: "",
	ONLOAD_TARGET: "",
	
	LANGUAGE: "en",
	LANUGAGE_ONDEMAND_RELOADING: true,
	LANGUAGE_SUPPORTED: [ 
		{encoding:"en", name:"English"},
		{encoding:"bg", name:"Български"},
		{encoding:"cs", name:"Česky"},
		{encoding:"de", name:"Deutsch"},
		{encoding:"dk", name:"Dansk"},
		{encoding:"es", name:"Español"},
		{encoding:"fi", name:"Suomi"},
		{encoding:"fr", name:"Français"},
		{encoding:"it", name:"Italiano"},
		{encoding:"lv", name:"Latviešu"},
		{encoding:"pl", name:"Polski"},
		{encoding:"ru", name:"Русский"}
	],
		
	TILE_SERVERS: [
		{	display_name: 'osm.org',
			url:'http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
			attribution:'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 Mapnik',
			options:{maxZoom: 18}
		},
		{	display_name: 'osm.de',
			url:'http://{s}.tile.openstreetmap.de/tiles/osmde/{z}/{x}/{y}.png',
			attribution:'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 Mapnik',
			options:{maxZoom: 18}
		},
		{	display_name: 'MapQuest',
			url:'http://otile{s}.mqcdn.com/tiles/1.0.0/osm/{z}/{x}/{y}.png',
			attribution:'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 MapQuest',
			options:{maxZoom: 18, subdomains: '1234'}
		},
		{	display_name: 'CloudMade',
			url:'http://{s}.tile.cloudmade.com/BC9A493B41014CAABB98F0471D759707/997/256/{z}/{x}/{y}.png',
			attribution:'Map data &copy; 2011 OpenStreetMap contributors, Imagery &copy; 2011 CloudMade',
			options:{maxZoom: 18}
		},
		{
			display_name: 'Bing Road',
			apikey:'AjCb2f6Azv_xt9c6pl_xok96bgAYrXQNctnG4o07sTj4iS9N68Za4B3pRJyeCjGr',	// please use your own apikey (http://msdn.microsoft.com/en-us/library/ff428642.aspx) 
			options:{type:"Road", minZoom: 1},
			bing:true,
		},
		{
			display_name: 'Bing Aerial',
			apikey:'AjCb2f6Azv_xt9c6pl_xok96bgAYrXQNctnG4o07sTj4iS9N68Za4B3pRJyeCjGr',	// please use your own apikey (http://msdn.microsoft.com/en-us/library/ff428642.aspx)
			options:{type:"Aerial", minZoom: 1},
			bing:true,
		}
	],

	NOTIFICATIONS: {
		LOCALIZATION:	1800000,	// 30min
		CLICKING: 		60000,		// 1min
		DRAGGING: 		120000,		// 2min 
		MAINTENANCE:	false
	},
	OVERRIDE_MAINTENANCE_NOTIFICATION_HEADER: undefined,
	OVERRIDE_MAINTENANCE_NOTIFICATION_BODY: undefined
};
