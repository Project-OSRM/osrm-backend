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
// [German language support]


OSRM.Localization["de"] = {
//gui
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
"GUI_START": "Start",
"GUI_END": "Ziel",
"GUI_RESET": "Reset",
"GUI_SEARCH": "Zeigen",
"GUI_REVERSE": "Umdrehen",
"GUI_OPTIONS": "Kartenwerkzeuge",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Unbenannte Straßen hervorheben",
"GUI_START_TOOLTIP": "Startposition eingeben",
"GUI_END_TOOLTIP": "Zielposition eingeben",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.osm.org/'>OSM</a>",
// geocoder
"SEARCH_RESULTS": "Suchergebnisse",
"FOUND_X_RESULTS": "%i Ergebnisse gefunden",
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
"E": "Ost",
"S": "Süden",
"W": "Westen",
"NE": "Nordost",
"SE": "Südost",
"SW": "Südwest",
"NW": "Nordwest",
// driving directions
"DIRECTION_0":"Unbekannte Anweisung[ auf <b>%s</b>]",
"DIRECTION_1":"Geradeaus weiterfahren[ auf <b>%s</b>]",
"DIRECTION_2":"Leicht rechts abbiegen[ auf <b>%s</b>]",
"DIRECTION_3":"Rechts abbiegen[ auf <b>%s</b>]",
"DIRECTION_4":"Scharf rechts abbiegen[ auf <b>%s</b>]",
"DIRECTION_5":"Wenden[ auf <b>%s</b>]",
"DIRECTION_6":"Leicht links abbiegen[ auf <b>%s</b>]",
"DIRECTION_7":"Links abbiegen[ auf <b>%s</b>]",
"DIRECTION_8":"Scharf links abbiegen[ auf <b>%s</b>]",
"DIRECTION_10":"Fahren Sie Richtung <b>%s</b>",
"DIRECTION_11-1":"In den Kreisverkehr einfahren und bei erster Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-2":"In den Kreisverkehr einfahren und bei zweiter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-3":"In den Kreisverkehr einfahren und bei dritter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-4":"In den Kreisverkehr einfahren und bei vierter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-5":"In den Kreisverkehr einfahren und bei fünfter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-6":"In den Kreisverkehr einfahren und bei sechster Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-7":"In den Kreisverkehr einfahren und bei siebter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-8":"In den Kreisverkehr einfahren und bei achter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-9":"In den Kreisverkehr einfahren und bei neunter Möglichkeit[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_11-x":"In den Kreisverkehr einfahren und bei einer der vielen Möglichkeiten[ in Richtung <b>%s</b>] verlassen",
"DIRECTION_15":"Sie haben Ihr Ziel erreicht"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING==true )
	OSRM.Localization.setLanguage("de");