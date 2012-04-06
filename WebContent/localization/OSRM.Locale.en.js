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
"NW": "northwest",
// driving directions
"DIRECTION_1":"Turn left[ on <b>%s</b>]",
"DIRECTION_2":"Turn right[ on <b>%s</b>]",
"DIRECTION_3":"U-Turn[ on <b>%s</b>]",
"DIRECTION_4":"Head %s",
"DIRECTION_5":"Continue[ on <b>%s</b>]",
"DIRECTION_6":"Turn slight left[ on <b>%s</b>]",
"DIRECTION_7":"Turn slight right[ on <b>%s</b>]",
"DIRECTION_8":"Turn sharp left[ on <b>%s</b>]",
"DIRECTION_9":"Turn sharp right[ on <b>%s</b>]",
"DIRECTION_10":"Enter roundabout and leave at first exit[ on <b>%s</b>]",
"DIRECTION_11":"Enter roundabout and leave at second exit[ on <b>%s</b>]",
"DIRECTION_12":"Enter roundabout and leave at third exit[ on <b>%s</b>]",
"DIRECTION_13":"Enter roundabout and leave at fourth exit[ on <b>%s</b>]",
"DIRECTION_14":"Enter roundabout and leave at fifth exit[ on <b>%s</b>]",
"DIRECTION_15":"Enter roundabout and leave at sixth exit[ on <b>%s</b>]",
"DIRECTION_16":"Enter roundabout and leave at seventh exit[ on <b>%s</b>]",
"DIRECTION_17":"Enter roundabout and leave at eighth exit[ on <b>%s</b>]",
"DIRECTION_18":"Enter roundabout and leave at nineth exit[ on <b>%s</b>]",
"DIRECTION_19":"Enter roundabout and leave at tenth exit[ on <b>%s</b>]",
"DIRECTION_20":"Enter roundabout and leave at one of the too many exits[ on <b>%s</b>]",
"DIRECTION_21":"You have reached your destination"
};

//set GUI language on load
OSRM.Localization.change("en");