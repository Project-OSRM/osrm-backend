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
"LANGUAGE": "English",
// gui
"GUI_START": "Start",
"GUI_END": "End",
"GUI_RESET": "&nbsp;&nbsp;Reset&nbsp;&nbsp;",
"GUI_ZOOM": "Zoom onto Route",
"GUI_SEARCH": "&nbsp;&nbsp;Show&nbsp;&nbsp;",
"GUI_REVERSE": "Reverse",
"GUI_START_TOOLTIP": "Enter start",
"GUI_END_TOOLTIP": "Enter destination",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.osm.org/'>OSM</a>",
// config
"GUI_CONFIGURATION": "Configuration",
"GUI_LANGUAGE": "Language",
"GUI_UNITS": "Units",
"GUI_KILOMETERS": "Kilometers",
"GUI_MILES": "Miles",
"GUI_DATA_TIMESTAMP": "data",
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
"DIRECTION_15":"You have reached your destination"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("en");
