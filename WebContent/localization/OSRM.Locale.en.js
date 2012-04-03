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
//gui
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
"GUI_START": "Start",
"GUI_END": "End",
"GUI_RESET": "&nbsp;&nbsp;Reset&nbsp;&nbsp;",
"GUI_SEARCH": "&nbsp;&nbsp;Show&nbsp;&nbsp;",
"GUI_REVERSE": "Reverse",
"GUI_OPTIONS": "Mapping Tools",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Highlight unnamed streets",
"GUI_START_TOOLTIP": "Enter start",
"GUI_END_TOOLTIP": "Enter destination",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.osm.org/'>OSM</a>",
// geocoder
"SEARCH_RESULTS": "Search Results",
"TIMED_OUT": "Timed Out",
"NO_RESULTS_FOUND": "No results found",
"NO_RESULTS_FOUND_SOURCE": "No results found for start",
"NO_RESULTS_FOUND_TARGET": "No results found for end",
//routing
"ROUTE_DESCRIPTION": "Route Description",
"GET_LINK_TO_ROUTE": "Generate Link",
"GENERATE_LINK_TO_ROUTE": "waiting for link",
"LINK_TO_ROUTE_TIMEOUT": "not available",
"GPX_FILE": "GPX File",
"DISTANCE": "Distance",
"DURATION": "Duration",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Your route is being computed",
"NO_ROUTE_FOUND": "No route possible",
// directions
"N": "north",
"E": "east",
"S": "south",
"W": "west",
"NE": "northeast",
"SE": "southeast",
"SW": "southwest",
"NW": "northwest"
};

//set GUI language on load
OSRM.GUI.setLanguage();