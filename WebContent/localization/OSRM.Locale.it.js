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
// [Italian language support]


OSRM.Localization["it"] = {
// own language
"LANGUAGE": "Italiano",
//gui
"GUI_START": "Partenza",
"GUI_END": "Destinazione",
"GUI_RESET": "Reset",
"GUI_ZOOM": "Zoom su Percorso",
"GUI_SEARCH": "Mostra",
"GUI_REVERSE": "Inverti",
"GUI_START_TOOLTIP": "Inserire la Partenza",
"GUI_END_TOOLTIP": "Inserire la destinazione",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.osm.org/'>OSM</a>",
//config
"GUI_CONFIGURATION": "Configurazione",
"GUI_LANGUAGE": "Lingua",
"GUI_UNITS": "Unità",
"GUI_KILOMETERS": "Chilometri",
"GUI_MILES": "Miles",
"GUI_DATA_TIMESTAMP": "data",
// mapping
"GUI_MAPPING_TOOLS": "Strumenti per la Mappatura",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Evidenzia strade senza nome",
"GUI_SHOW_PREVIOUS_ROUTES": "Mostra le percorsi precedenti",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Risultati della ricerca",
"FOUND_X_RESULTS": "trovati %i risultati",
"TIMED_OUT": "Timeout",
"NO_RESULTS_FOUND": "Nessun risultato trovato",
"NO_RESULTS_FOUND_SOURCE": "Nessun risultato trovato per la partenza",
"NO_RESULTS_FOUND_TARGET": "Nessun risultato trovato per la destinazione",
//routing
"ROUTE_DESCRIPTION": "Descrizione del percorso",
"GET_LINK_TO_ROUTE": "Genera un Link",
"GENERATE_LINK_TO_ROUTE": "in attesa del link",
"LINK_TO_ROUTE_TIMEOUT": "non disponibile",
"GPX_FILE": "File GPX",
"DISTANCE": "Distanza",
"DURATION": "Durata",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Sto calcolando il tuo percorso",
"NO_ROUTE_FOUND": "Nessun percorso possibile",
//printing
"OVERVIEW_MAP": "Mappa d'insieme",
// directions
"N": "nord",
"E": "est",
"S": "sud",
"W": "ovest",
"NE": "nordest",
"SE": "sudest",
"SW": "sudovest",
"NW": "nordovest",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Istruzione sconosciuta[ su <b>%s</b>]",
"DIRECTION_1":"Continuare[ su <b>%s</b>]",
"DIRECTION_2":"Girare leggermente a destra[ su <b>%s</b>]",
"DIRECTION_3":"Girare a destra[ su <b>%s</b>]",
"DIRECTION_4":"Girare decisamente a destra[ su <b>%s</b>]",
"DIRECTION_5":"Compire una inversione ad U[ su <b>%s</b>]",
"DIRECTION_6":"Girare decisamente a sinistra[ su <b>%s</b>]",
"DIRECTION_7":"Girare a sinistra[ su <b>%s</b>]",
"DIRECTION_8":"Girare leggermente a sinistra[ su <b>%s</b>]",
"DIRECTION_10":"Dirigersi a <b>%d</b>[ su <b>%s</b>]",
"DIRECTION_11-1":"Immettersi nella rotonda ed abbandonarla alla prima uscita[ su <b>%s</b>]",
"DIRECTION_11-2":"Immettersi nella rotonda ed abbandonarla alla seconda uscita[ su <b>%s</b>]",
"DIRECTION_11-3":"Immettersi nella rotonda ed abbandonarla alla terza uscita[ su <b>%s</b>]",
"DIRECTION_11-4":"Immettersi nella rotonda ed abbandonarla alla quarta uscita[ su <b>%s</b>]",
"DIRECTION_11-5":"Immettersi nella rotonda ed abbandonarla alla quinta uscita[ su <b>%s</b>]",
"DIRECTION_11-6":"Immettersi nella rotonda ed abbandonarla alla sesta uscita[ su <b>%s</b>]",
"DIRECTION_11-7":"Immettersi nella rotonda ed abbandonarla alla settima uscita[ su <b>%s</b>]",
"DIRECTION_11-8":"Immettersi nella rotonda ed abbandonarla alla ottava uscita[ su <b>%s</b>]",
"DIRECTION_11-9":"Immettersi nella rotonda ed abbandonarla alla nona uscita[ su <b>%s</b>]",
"DIRECTION_11-x":"Immettersi nella rotonda ed abbandonarla ad una delle tante uscite[ su <b>%s</b>]",
"DIRECTION_15":"Hai raggiunto la tua destinazione"
};

//set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("it");
