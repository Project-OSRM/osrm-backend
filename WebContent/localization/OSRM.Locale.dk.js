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
// [Danish language support]


OSRM.Localization["dk"] = {
// own language
"CULTURE": "da-DK",
"LANGUAGE": "Dansk",
// gui
"GUI_START": "Start",
"GUI_END": "Destination",
"GUI_RESET": "Nulstil",
"GUI_ZOOM_ON_ROUTE": "Zoom på Rute",
"GUI_ZOOM_ON_USER": "Zoom på Bruger",
"GUI_SEARCH": "Vis",
"GUI_REVERSE": "Omvendt",
"GUI_START_TOOLTIP": "Indtast start",
"GUI_END_TOOLTIP": "Indtast destination",
"GUI_MAIN_WINDOW": "Hovedvinduet",
"GUI_ZOOM_IN": "Zoome ind",
"GUI_ZOOM_OUT": "Zoome ud",
// config
"GUI_CONFIGURATION": "Konfiguration",
"GUI_LANGUAGE": "Sprog",
"GUI_UNITS": "Enheder",
"GUI_KILOMETERS": "Kilometer",
"GUI_MILES": "Miles",
// mapping
"GUI_MAPPING_TOOLS": "Kortlægnings værktøjer",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Fremhæv unavngivne veje",
"GUI_SHOW_PREVIOUS_ROUTES": "Vis tidligere ruter",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Søgeresultater",
"FOUND_X_RESULTS": "fandt %i resultater",
"TIMED_OUT": "Indtet svar",
"NO_RESULTS_FOUND": "Ingen resultater",
"NO_RESULTS_FOUND_SOURCE": "Ingen resultater for start",
"NO_RESULTS_FOUND_TARGET": "Ingen resultater for destination",
// routing
"ROUTE_DESCRIPTION": "Rutebeskrivelse",
"GET_LINK_TO_ROUTE": "Lav link",
"GENERATE_LINK_TO_ROUTE": "venter på link",
"LINK_TO_ROUTE_TIMEOUT": "ikke tilgængelig",
"GPX_FILE": "GPX Fil",
"DISTANCE": "Distance",
"DURATION": "Varighed",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Din rute bliver beregnet",
"NO_ROUTE_FOUND": "Ingen mulig rute fundet",
// printing
"OVERVIEW_MAP": "Oversigtskort",
"NO_ROUTE_SELECTED": "Ikke valgte rute",
// routing engines
"ENGINE_0": "Bil (hurtigste)",
// directions
"N": "nord",
"E": "øst",
"S": "syd",
"W": "vest",
"NE": "nordøst",
"SE": "sydøst",
"SW": "sydvest",
"NW": "nordvest",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Unknown instruction[ on <b>%s</b>]",
"DIRECTION_1":"Fortsæt [ ad <b>%s</b>]",
"DIRECTION_2":"Drej svagt til højre [ ad <b>%s</b>]",
"DIRECTION_3":"Drej til højre[ ad <b>%s</b>]",
"DIRECTION_4":"Drej skarpt til højre[ ad <b>%s</b>]",
"DIRECTION_5":"U-vending[ ad <b>%s</b>]",
"DIRECTION_6":"Drej skarpt til venstre[ ad <b>%s</b>]",
"DIRECTION_7":"Drej til venstre[ ad <b>%s</b>]",
"DIRECTION_8":"Drej svagt til venstre[ ad <b>%s</b>]",
"DIRECTION_10":"Kør mod <b>%d</b>[ ad <b>%s</b>]",
"DIRECTION_11-1":"Kør ind i rundkørslen og tag første udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-2":"Kør ind i rundkørslen og tag anden udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-3":"Kør ind i rundkørslen og tag tredje udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-4":"Kør ind i rundkørslen og tag fjerde udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-5":"Kør ind i rundkørslen og tag femte udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-6":"Kør ind i rundkørslen og tag sjette udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-7":"Kør ind i rundkørslen og tag syvende udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-8":"Kør ind i rundkørslen og tag ottende udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-9":"Kør ind i rundkørslen og tag niende udkørsel[ ad <b>%s</b>]",
"DIRECTION_11-x":"Kør ind i rundkørslen og tag en af de alt for mange udkørsler[ ad <b>%s</b>]",
"DIRECTION_15":"Du er ankommet til din destination"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("dk");
