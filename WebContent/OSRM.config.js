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
	HOST_ROUTING_URL: 'http://router.project-osrm.org/viaroute',
	HOST_SHORTENER_URL: 'http://map.project-osrm.org/shorten/',
	HOST_TIMESTAMP_URL: 'http://router.project-osrm.org/timestamp',
	HOST_GEOCODER_URL: 'http://nominatim.openstreetmap.org/search',
	HOST_REVERSE_GEOCODER_URL: 'http://nominatim.openstreetmap.org/reverse',
	WEBSITE_URL: document.URL.replace(/#*(\?.*|$)/i,""),					// truncates URL before first ?, and removes tailing #
	JSONP_TIMEOUT: 10000,
	ZOOM_LEVEL: 14,
	ONLOAD_ZOOM_LEVEL: 5,
	ONLOAD_LATITUDE: 48.84,
	ONLOAD_LONGITUDE: 10.10,
	ONLOAD_SOURCE: "",
	ONLOAD_TARGET: "",
	HIGHLIGHT_ZOOM_LEVEL: 16,
	DISTANCE_FORMAT: 0,														// 0: km, 1: miles
	GEOCODER_BOUNDS: '',													// the world is not enough!
	//GEOCODER_BOUNDS: '&bounded=1&viewbox=-27.0,72.0,46.0,36.0',			// bounds for Europe
	
	SHORTENER_PARAMETERS: '%url&jsonp=%jsonp',
	SHORTENER_REPLY_PARAMETER: 'ShortURL',
	
	LANGUAGE: "en",
	LANUGAGE_ONDEMAND_RELOADING: true,
	LANGUAGE_SUPPORTED: [ 
		{encoding:"en", name:"English"},
		{encoding:"de", name:"Deutsch"},
		{encoding:"dk", name:"Dansk"},
		{encoding:"fi", name:"Suomi"},
		{encoding:"fr", name:"Français"},
		{encoding:"it", name:"Italiano"},
		{encoding:"pl", name:"Polski", culture:"en-US"}
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
			type:"Road",
			options:{minZoom: 1},
			bing:true,
		},
		{
			display_name: 'Bing Aerial',
			apikey:'AjCb2f6Azv_xt9c6pl_xok96bgAYrXQNctnG4o07sTj4iS9N68Za4B3pRJyeCjGr',	// please use your own apikey (http://msdn.microsoft.com/en-us/library/ff428642.aspx)
			type:"Aerial",
			options:{minZoom: 1},
			bing:true,
		}
	],
	
	MAINTENANCE: true,
	MAINTENANCE_HEADER: "Scheduled Maintenance",
	MAINTENANCE_TEXT: "The OSRM Website is down for a scheduled maintenance. Please be patient while required updates are performed. The site will be back online shortly.<br/><br/>In the meantime you may want to go out an map a friendly neighborhood near you...<br/><br/><br/>[OSRM]",	
};