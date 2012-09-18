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

// OSRM localization
// [English language support]


OSRM.Localization["en"] = {
// own language
"CULTURE": "en-US",
"LANGUAGE": "English",
// gui
"GUI_START": "Start",
"GUI_END": "End",
"GUI_RESET": "&nbsp;&nbsp;Reset&nbsp;&nbsp;",
"GUI_ZOOM_ON_ROUTE": "Zoom onto Route",
"GUI_ZOOM_ON_USER": "Zoom onto User",
"GUI_SEARCH": "&nbsp;&nbsp;Show&nbsp;&nbsp;",
"GUI_REVERSE": "Reverse",
"GUI_START_TOOLTIP": "Enter start",
"GUI_END_TOOLTIP": "Enter destination",
"GUI_MAIN_WINDOW": "Main window",
"GUI_ZOOM_IN": "Zoom in",
"GUI_ZOOM_OUT": "Zoom out",
// config
"GUI_CONFIGURATION": "Configuration",
"GUI_LANGUAGE": "Language",
"GUI_UNITS": "Units",
"GUI_KILOMETERS": "Kilometers",
"GUI_MILES": "Miles",
// mapping
"GUI_MAPPING_TOOLS": "Mapping Tools",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Highlight unnamed streets",
"GUI_SHOW_PREVIOUS_ROUTES": "Show previous routes",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Search Results",
"FOUND_X_RESULTS": "found %i results",
"TIMED_OUT": "Timed Out",
"NO_RESULTS_FOUND": "No results found",
"NO_RESULTS_FOUND_SOURCE": "No results found for start",
"NO_RESULTS_FOUND_TARGET": "No results found for end",
// routing
"ROUTE_DESCRIPTION": "Route Description",
"GET_LINK_TO_ROUTE": "Generate Link",
"GENERATE_LINK_TO_ROUTE": "waiting for link",
"LINK_TO_ROUTE_TIMEOUT": "not available",
"GPX_FILE": "GPX File",
"DISTANCE": "Distance",
"DURATION": "Duration",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Your route is being computed",
"NO_ROUTE_FOUND": "No route possible",
// printing
"OVERVIEW_MAP": "Overview Map",
"NO_ROUTE_SELECTED": "No route selected",
// routing engines
"ENGINE_0": "Car (fastest)",
// directions
"N": "north",
"E": "east",
"S": "south",
"W": "west",
"NE": "northeast",
"SE": "southeast",
"SW": "southwest",
"NW": "northwest",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Unknown instruction[ onto <b>%s</b>]",
"DIRECTION_1":"Continue[ onto <b>%s</b>]",
"DIRECTION_2":"Turn slight right[ onto <b>%s</b>]",
"DIRECTION_3":"Turn right[ onto <b>%s</b>]",
"DIRECTION_4":"Turn sharp right[ onto <b>%s</b>]",
"DIRECTION_5":"U-Turn[ onto <b>%s</b>]",
"DIRECTION_6":"Turn sharp left[ onto <b>%s</b>]",
"DIRECTION_7":"Turn left[ onto <b>%s</b>]",
"DIRECTION_8":"Turn slight left[ onto <b>%s</b>]",
"DIRECTION_10":"Head <b>%d</b>[ onto <b>%s</b>]",
"DIRECTION_11-1":"Enter roundabout and leave at first exit[ onto <b>%s</b>]",
"DIRECTION_11-2":"Enter roundabout and leave at second exit[ onto <b>%s</b>]",
"DIRECTION_11-3":"Enter roundabout and leave at third exit[ onto <b>%s</b>]",
"DIRECTION_11-4":"Enter roundabout and leave at fourth exit[ onto <b>%s</b>]",
"DIRECTION_11-5":"Enter roundabout and leave at fifth exit[ onto <b>%s</b>]",
"DIRECTION_11-6":"Enter roundabout and leave at sixth exit[ onto <b>%s</b>]",
"DIRECTION_11-7":"Enter roundabout and leave at seventh exit[ onto <b>%s</b>]",
"DIRECTION_11-8":"Enter roundabout and leave at eighth exit[ onto <b>%s</b>]",
"DIRECTION_11-9":"Enter roundabout and leave at nineth exit[ onto <b>%s</b>]",
"DIRECTION_11-x":"Enter roundabout and leave at one of the too many exits[ onto <b>%s</b>]",
"DIRECTION_15":"You have reached your destination",
// notifications
"NOTIFICATION_MAINTENANCE_HEADER":	"Scheduled Maintenance",
"NOTIFICATION_MAINTENANCE_BODY":	"The OSRM Website is down for a scheduled maintenance. " + 
									"Please be patient while required updates are performed. " +
									"The site will be back online shortly." +
									"<br/><br/>" + 
									"In the meantime you may want to go out an map a friendly neighborhood near you..." + 
									"<br/><br/><br/>[OSRM]",
"NOTIFICATION_LOCALIZATION_HEADER":	"Did you know? You can change the display language.",
"NOTIFICATION_LOCALIZATION_BODY":	"You can use the pulldown menu in the upper left corner to select your favorite language. " +
									"<br/><br/>" +
									"Don't despair if you cannot find your language of choice. " +
									"If you want, you can help to provide additional translations! " +
									"Visit <a href='https://github.com/DennisSchiefer/Project-OSRM-Web'>here</a> for more information.",
"NOTIFICATION_CLICKING_HEADER":		"Did you know? You can click on the map to set route markers.",
"NOTIFICATION_CLICKING_BODY":		"You can click on the map with the left mouse button to set a source marker (green) or a target marker (red), " +
									"if the source marker already exists. " +
									"The address of the selected location will be displayed in the boxes to the left. " + 
									"<br/><br/>" +
									"You can delete a marker by clicking on it again with the left mouse button.",
"NOTIFICATION_DRAGGING_HEADER":		"Did you know? You can drag each route marker on the map.",
"NOTIFICATION_DRAGGING_BODY":		"You can drag a marker by clicking on it with the left mouse button and holding the button pressed. " +
									"Then you can move the marker around the map and the route will be updated instantaneously. " +
									"<br/><br/>" +
									"You can even create intermediate markers by dragging them off of the main route! ",
// do not translate below this line
"GUI_LEGAL_NOTICE": "Routing by <a href='http://project-osrm.org/'>Project OSRM</a> - Geocoder by <a href='http://wiki.openstreetmap.org/wiki/Nominatim'>Nominatim</a> - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a>",
"GUI_DATA_TIMESTAMP": "data: ",
"GUI_VERSION": "gui: ",
"QR": "QR"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("en");
