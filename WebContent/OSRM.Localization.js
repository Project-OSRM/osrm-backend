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
// [basic localization options]


OSRM.Localization = {
		
// if existing, return localized string -> English string -> input string
translate: function(text) {
	if( OSRM.Localization[OSRM.DEFAULTS.LANGUAGE][text] )
		return OSRM.Localization[OSRM.DEFAULTS.LANGUAGE][text];
	else if( OSRM.Localization["en"][text] )
		return OSRM.Localization["en"][text];
	else
		return text;
}
};

// shorter call to translate function
OSRM.loc = OSRM.Localization.translate;


// German language support
OSRM.Localization["de"] = {
//gui
"GUI_START": "Start",
"GUI_END": "Ziel",
"GUI_RESET": "Reset",
"GUI_SEARCH": "Zeigen",
"GUI_REVERSE": "Umdrehen",
"GUI_OPTIONS": "Optionen",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Unbenannte Straßen hervorheben",
"GUI_START_TOOLTIP": "Startposition eingeben",
"GUI_END_TOOLTIP": "Zielposition eingeben",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.osm.org/'>OSM</a>",
// geocoder
"SEARCH_RESULTS": "Suchergebnisse",
"TIMED_OUT": "Zeitüberschreitung",
"NO_RESULTS_FOUND": "Keine Ergebnisse gefunden",
"NO_RESULTS_FOUND_SOURCE": "Keine Ergebnisse gefunden für Start",
"NO_RESULTS_FOUND_TARGET": "Keine Ergebnisse gefunden für Ziel",
// routing
"ROUTE_DESCRIPTION": "Routenbeschreibung",
"GET_LINK_TO_ROUTE": "Generiere Link",
"GENERATE_LINK_TO_ROUTE": "Warte auf Antwort",
"LINK_TO_ROUTE_TIMEOUT": "nicht möglich",
"GPX_FILE": "GPX Datei",
"DISTANCE": "Distanz",
"DURATION": "Dauer",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Ihre Route wird berechnet",
"NO_ROUTE_FOUND": "Keine Route hierher möglich",
// directions
"N": "Norden",
"O": "Ost",
"S": "Süden",
"W": "Westen",
"NO": "Nordost",
"SO": "Südost",
"SW": "Südwest",
"NW": "Nordwest"
};


// English language support
OSRM.Localization["en"] = {
//gui
"GUI_START": "Start",
"GUI_END": "End",
"GUI_RESET": "&nbsp;&nbsp;Reset&nbsp;&nbsp;",
"GUI_SEARCH": "&nbsp;&nbsp;Show&nbsp;&nbsp;",
"GUI_REVERSE": "Reverse",
"GUI_OPTIONS": "Options",
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