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


OSRM.Localization["ro"] = {
// own language
"CULTURE": "ro-RO",
"LANGUAGE": "Română",
// gui
"GUI_START": "începe",
"GUI_END": "țintă",
"GUI_RESET": "Resetare",
"GUI_ZOOM_ON_ROUTE": "Zoom pe ruta",
"GUI_ZOOM_ON_USER": "Zoom pe utilizator",
"GUI_SEARCH": "Arata",
"GUI_REVERSE": "Inverseaza",
"GUI_START_TOOLTIP": "Introduceti punctul de pornire",
"GUI_END_TOOLTIP": "Introduceti destinatia",
"GUI_MAIN_WINDOW": "Fereastra principală",
"GUI_ZOOM_IN": "zoom in",
"GUI_ZOOM_OUT": "zoom out",
// config
"GUI_CONFIGURATION": "Configurare",
"GUI_LANGUAGE": "Limba",
"GUI_UNITS": "Unitati",
"GUI_KILOMETERS": "Kilometri",
"GUI_MILES": "Mile",
// mapping
"GUI_MAPPING_TOOLS": "Instrumente de cartografiere",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Evidentiaza strazile fara nume",
"GUI_SHOW_PREVIOUS_ROUTES": "Arata trasee anterioare",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Rezultatele cautarii",
"FOUND_X_RESULTS": "Rezultatele gasite",
"TIMED_OUT": "Timpul a expirat",
"NO_RESULTS_FOUND": "Nu s-au gasit  rezultate",
"NO_RESULTS_FOUND_SOURCE": "Nu s-au gasit rezultate pentru punctul de plecare",
"NO_RESULTS_FOUND_TARGET": "Nu s-au gasit rezultate pentru punctul de sosire",
// routing
"ROUTE_DESCRIPTION": "Descrierea rutei",
"GET_LINK_TO_ROUTE": "Genereaza link",
"GENERATE_LINK_TO_ROUTE": "in asteptare pentru link",
"LINK_TO_ROUTE_TIMEOUT": "nu este disponibil",
"GPX_FILE": "GPX File",
"DISTANCE": "Distanta",
"DURATION": "Durata",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Ruta dumneavoastra se calculeaza",
"NO_ROUTE_FOUND": "Nicio ruta nu este posibila",
// printing
"OVERVIEW_MAP": "Overview Map",
"NO_ROUTE_SELECTED": "Nu este selectata nicio ruta",
// routing engines
"ENGINE_0": "Masina (cel mai rapid)",
// directions
"N": "nord",
"E": "est",
"S": "sud",
"W": "vest",
"NE": "nordest",
"SE": "sudest",
"SW": "sudvest",
"NW": "nordvest",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":" Instructiuni necunoscute[ spre <b>%s</b>]",
"DIRECTION_1":"Continuati[ spre <b>%s</b>]",
"DIRECTION_2":"Faceti usor dreapta[ spre <b>%s</b>]",
"DIRECTION_3":"Faceti dreapta[ spre <b>%s</b>]",
"DIRECTION_4":"Faceti brusc dreapta[ spre <b>%s</b>]",
"DIRECTION_5":"Intoarcere in U[ spre <b>%s</b>]",
"DIRECTION_6":"Faceti brusc stanga[ spre <b>%s</b>]",
"DIRECTION_7":"Faceti stanga[ spre <b>%s</b>]",
"DIRECTION_8":"Faceti usor stanga[ spre <b>%s</b>]",
"DIRECTION_10":"Indreptati-va <b>%d</b>[ spre <b>%s</b>]",
"DIRECTION_11-1":"Intrati in sensul giratoriu si alegeti prima iesire[ spre <b>%s</b>]",
"DIRECTION_11-2":"Intrati in sensul giratoriu si alegeti a doua iesire[ spre <b>%s</b>]",
"DIRECTION_11-3":"Intrati in sensul giratoriu si alegeti a treia iesire[ spre <b>%s</b>]",
"DIRECTION_11-4":"Intrati in sensul giratoriu si alegeti a patra iesire[ spre <b>%s</b>]",
"DIRECTION_11-5":"Intrati in sensul giratoriu si alegeti a cincea iesire[ spre <b>%s</b>]",
"DIRECTION_11-6":"Intrati in sensul giratoriu si alegeti a sasea iesire[ spre <b>%s</b>]",
"DIRECTION_11-7":"Intrati in sensul giratoriu si alegeti a saptea iesire[ spre <b>%s</b>]",
"DIRECTION_11-8":"Intrati in sensul giratoriu si alegeti a opta iesire[ spre <b>%s</b>]",
"DIRECTION_11-9":"Intrati in sensul giratoriu si alegeti a noua iesire[ spre <b>%s</b>]",
"DIRECTION_11-x":"Intrati in sensul giratoriu si alegeti una din multele iesiri[ spre <b>%s</b>]",
"DIRECTION_15":"Ati ajuns la destinatie"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING==true )
	OSRM.Localization.setLanguage("ro");
