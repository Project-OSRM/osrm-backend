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
// [Czech language support]


OSRM.Localization["cs"] = {
// own language
"CULTURE": "cs-CZ",
"LANGUAGE": "česky", 		
//gui
"GUI_START": "Odkud",
"GUI_END": "Kam",
"GUI_RESET": "Vyčistit",
"GUI_ZOOM": "Zoom na trasu",
"GUI_SEARCH": "Ukázat",
"GUI_REVERSE": "Prohodit",
"GUI_START_TOOLTIP": "Zadejte začátek cesty",
"GUI_END_TOOLTIP": "Zadejte cíl cesty",
//config
"GUI_CONFIGURATION": "Nastavení",
"GUI_LANGUAGE": "Jazyk",
"GUI_UNITS": "Jednotky",
"GUI_KILOMETERS": "kilometry",
"GUI_MILES": "míle",
// mapping
"GUI_MAPPING_TOOLS": "Mapovací nástroje",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Zvýraznit nepojmenované ulice",
"GUI_SHOW_PREVIOUS_ROUTES": "Zobrazit předchozí trasy",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Výsledky hledání",
"FOUND_X_RESULTS": "nalezeno %i výsledků",
"TIMED_OUT": "Časová lhůta uplynula",
"NO_RESULTS_FOUND": "Nejsou žádné výsledky",
"NO_RESULTS_FOUND_SOURCE": "Nejsou žádné výsledky pro začátek trasy",
"NO_RESULTS_FOUND_TARGET": "Nejsou žádné výsledky pro konec trasy",
//routing
"ROUTE_DESCRIPTION": "Popis trasy",
"GET_LINK_TO_ROUTE": "Generovat odkaz",
"GENERATE_LINK_TO_ROUTE": "čekání na odkaz",
"LINK_TO_ROUTE_TIMEOUT": "není k dispozici",
"GPX_FILE": "GPX soubor",
"DISTANCE": "Vzdálenost",
"DURATION": "Doba",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Vaše trasa byla vyznačena",
"NO_ROUTE_FOUND": "Trasu nelze vyznačit",
//printing
"OVERVIEW_MAP": "Přehledová mapka",
"NO_ROUTE_SELECTED": "Ne vybranou trasu",
// directions
"N": "sever",
"E": "východ",
"S": "jih",
"W": "západ",
"NE": "severovýchod",
"SE": "jihovýchod",
"SW": "jihozápad",
"NW": "severozápad",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Neznámý pokyn[ na <b>%s</b>]",
"DIRECTION_1":"Pokračujte[ silnicí <b>%s</b>]",
"DIRECTION_2":"Zahněte mírně vpravo[ na silnici <b>%s</b>]",
"DIRECTION_3":"Zahněte vpravo[ na silnici <b>%s</b>]",
"DIRECTION_4":"Zahněte ostře doprava[ na silnici <b>%s</b>]",
"DIRECTION_5":"Otočte se[ na silnici <b>%s</b>]",
"DIRECTION_6":"Zahněte ostře doleva[ na silnici <b>%s</b>]",
"DIRECTION_7":"Zahněte vlevo[ na silnici <b>%s</b>]",
"DIRECTION_8":"Zahněte mírně vlevo[ na silnici <b>%s</b>]",
"DIRECTION_10":"Jeďte na <b>%d</b>[ silnicí <b>%s</b>]",
"DIRECTION_11-1":"Najeďte na kruhový objezd a opusťte jej prvním výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-2":"Najeďte na kruhový objezd a opusťte jej druhým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-3":"Najeďte na kruhový objezd a opusťte jej třetím výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-4":"Najeďte na kruhový objezd a opusťte jej čtvrtým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-5":"Najeďte na kruhový objezd a opusťte jej pátým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-6":"Najeďte na kruhový objezd a opusťte jej šestým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-7":"Najeďte na kruhový objezd a opusťte jej sedmým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-8":"Najeďte na kruhový objezd a opusťte jej osmým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-9":"Najeďte na kruhový objezd a opusťte jej devátým výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_11-1":"Najeďte na kruhový objezd a opusťte jej vámi vybraným výjezdem[ na silnici <b>%s</b>]",
"DIRECTION_15":"Jste u cíle"
};

//set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("cs");
