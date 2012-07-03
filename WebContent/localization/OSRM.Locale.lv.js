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
// [Latvian language support]


OSRM.Localization["lv"] = {
// own language
"LANGUAGE": "Latviešu",
// gui
"GUI_START": "Sākums",
"GUI_END": "Galamērķis",
"GUI_RESET": "&nbsp;&nbsp;Reset&nbsp;&nbsp;",
"GUI_ZOOM": "Padidinti ant Route",
"GUI_SEARCH": "&nbsp;&nbsp;Rādīt&nbsp;&nbsp;",
"GUI_REVERSE": "Pretējā&nbsp;virzienā",
"GUI_START_TOOLTIP": "Izvēlieties sākumu",
"GUI_END_TOOLTIP": "Izvēlieties galamērķi",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.osm.org/'>OSM</a>",
// config
"GUI_CONFIGURATION": "Konfigurācija",
"GUI_LANGUAGE": "Valoda",
"GUI_UNITS": "Mērvienība",
"GUI_KILOMETERS": "Kilometri",
"GUI_MILES": "Jūdzes",
"GUI_DATA_TIMESTAMP": "data",
// mapping
"GUI_MAPPING_TOOLS": "Kartēšanas rīki",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Izcelt ielas bez nosaukumiem",
"GUI_SHOW_PREVIOUS_ROUTES": "Rādīt vēsturiskos maršrutus",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Meklēšanas rezultāti",
"FOUND_X_RESULTS": "atrasti %i ieraksti",
"TIMED_OUT": "Iestājās noildze",
"NO_RESULTS_FOUND": "Neizdevās atrast šādu maršrutu",
"NO_RESULTS_FOUND_SOURCE": "Šāds sākuma punkts netika atrasts",
"NO_RESULTS_FOUND_TARGET": "Šāds galamērķis netika atrasts",
// routing
"ROUTE_DESCRIPTION": "Maršruta apraksts",
"GET_LINK_TO_ROUTE": "Izveidot saiti",
"GENERATE_LINK_TO_ROUTE": "notiek saites veidošana",
"LINK_TO_ROUTE_TIMEOUT": "saite nav pieejama",
"GPX_FILE": "GPX fails",
"DISTANCE": "Attālums",
"DURATION": "Ilgums",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Tiek veikta maršruta aprēķināšana",
"NO_ROUTE_FOUND": "Maršrutu nav iespējams aprēķināt",
// printing
"OVERVIEW_MAP": "Kartes pārskats",
"NO_ROUTE_SELECTED": "Nav norādīts maršruts",
// directions
"N": "ziemeļu",
"E": "austrumu",
"S": "dienvidu",
"W": "rietumu",
"NE": "ziemeļaustrumu",
"SE": "dienvidaustrumu",
"SW": "dienvidrietumu",
"NW": "ziemeļrietumu",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Nezinama instrukcija[ uz <b>%s</b>]",
"DIRECTION_1":"Turpiniet ceļu[ pa <b>%s</b>]",
"DIRECTION_2":"Pagriezieties nedaudz pa labi [ uz <b>%s</b>]",
"DIRECTION_3":"Pagriezieties pa labi[ uz <b>%s</b>]",
"DIRECTION_4":"Pagriezieties strauji pa labi[ uz <b>%s</b>]",
"DIRECTION_5":"U-veida pagrieziens[ uz <b>%s</b>]",
"DIRECTION_6":"Pagriezieties strauji pa kreisi[ uz <b>%s</b>]",
"DIRECTION_7":"Pagriezieties pa kreisi[ uz <b>%s</b>]",
"DIRECTION_8":"Pagriezieties nedaudz pa kreisi[ uz <b>%s</b>]",
"DIRECTION_10":"Dotieties <b>%d</b> virzienā[ uz <b>%s</b>]",
"DIRECTION_11-1":"Iebrauciet aplī, brauciet pa pirmo izeju[ uz <b>%s</b>]",
"DIRECTION_11-2":"Iebrauciet aplī, brauciet pa otro izeju[ uz <b>%s</b>]",
"DIRECTION_11-3":"Iebrauciet aplī, brauciet pa trešo izeju[ uz <b>%s</b>]",
"DIRECTION_11-4":"Iebrauciet aplī, brauciet pa ceturto izeju[ uz <b>%s</b>]",
"DIRECTION_11-5":"Iebrauciet aplī, brauciet pa piekto izeju[ uz <b>%s</b>]",
"DIRECTION_11-6":"Iebrauciet aplī, brauciet pa sesto izeju[ uz <b>%s</b>]",
"DIRECTION_11-7":"Iebrauciet aplī, brauciet pa septīto izeju[ uz <b>%s</b>]",
"DIRECTION_11-8":"Iebrauciet aplī, brauciet pa astoto izeju[ uz <b>%s</b>]",
"DIRECTION_11-9":"Iebrauciet aplī, brauciet pa devīto izeju[ uz <b>%s</b>]",
"DIRECTION_11-x":"Ieprauciet aplī, brauciet pa vienu no pārāk daudzajām izejām[ uz <b>%s</b>]",
"DIRECTION_15":"Sasniegts galamērķis"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("lv");
