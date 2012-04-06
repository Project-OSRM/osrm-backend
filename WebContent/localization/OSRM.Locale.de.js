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
"NW": "Nordwest",
// driving directions
"DIRECTION_1":"Links abbiegen[ auf <b>%s</b>]",
"DIRECTION_2":"Rechts abbiegen[ auf <b>%s</b>]",
"DIRECTION_3":"Umkehren[ auf <b>%s</b>]",
"DIRECTION_4":"Fahren Sie Richtung %s",
"DIRECTION_5":"Weiterfahren[ auf <b>%s</b>]",
"DIRECTION_6":"Leicht links abbiegen[ auf <b>%s</b>]",
"DIRECTION_7":"Leicht rechts abbiegen[ auf <b>%s</b>]",
"DIRECTION_8":"Scharf links abbiegen[ auf <b>%s</b>]",
"DIRECTION_9":"Scharf rechts abbiegen[ auf <b>%s</b>]",
"DIRECTION_10":"In den Kreisverkehr einfahren und bei erster Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_11":"In den Kreisverkehr einfahren und bei zweiter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_12":"In den Kreisverkehr einfahren und bei dritter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_13":"In den Kreisverkehr einfahren und bei vierter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_14":"In den Kreisverkehr einfahren und bei f�nfter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_15":"In den Kreisverkehr einfahren und bei sechster Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_16":"In den Kreisverkehr einfahren und bei siebter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_17":"In den Kreisverkehr einfahren und bei achter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_18":"In den Kreisverkehr einfahren und bei neunter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_19":"In den Kreisverkehr einfahren und bei zehnter Möglichkeit verlassen[ auf <b>%s</b>]",
"DIRECTION_20":"In den Kreisverkehr einfahren und bei einer der vielen Möglichkeiten verlassen[ auf <b>%s</b>]",
"DIRECTION_21":"Sie haben Ihr Ziel erreicht"
};

// set GUI language on load
OSRM.Localization.change("de");