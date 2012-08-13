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
// [Polish language support]


OSRM.Localization["pl"] = {
// own language
"CULTURE": "pl-PL",
"LANGUAGE": "Polski",		
//gui
"GUI_START": "Początek",
"GUI_END": "Koniec",
"GUI_RESET": "Reset",
"GUI_ZOOM": "Zoom na Trasy",
"GUI_SEARCH": "Pokaż",
"GUI_REVERSE": "Odwróć",
"GUI_START_TOOLTIP": "Wprowadź początek",
"GUI_END_TOOLTIP": "Wprowadź koniec",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM hosting: <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder: <a href='http://www.osm.org/'>OSM</a>",
//config
"GUI_CONFIGURATION": "Konfiguracja",
"GUI_LANGUAGE": "Język",
"GUI_UNITS": "Jednostki",
"GUI_KILOMETERS": "Kilometrów",
"GUI_MILES": "Miles",
"GUI_DATA_TIMESTAMP": "data",
// mapping
"GUI_MAPPING_TOOLS": "Narzędzia mapowania",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Zaznacz ulice bez nazwy",
"GUI_SHOW_PREVIOUS_ROUTES": "Pokaż poprzednie trasy",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Wyniki wyszukiwania",
"FOUND_X_RESULTS": "znaleziono %i wyników",
"TIMED_OUT": "Upłynął czas oczekiwania",
"NO_RESULTS_FOUND": "Brak wyników",
"NO_RESULTS_FOUND_SOURCE": "Brak wyników dla początku trasy",
"NO_RESULTS_FOUND_TARGET": "Brak wyników dla końca trasy",
//routing
"ROUTE_DESCRIPTION": "Opis trasy",
"GET_LINK_TO_ROUTE": "Generuj link",
"GENERATE_LINK_TO_ROUTE": "oczekiwanie na link",
"LINK_TO_ROUTE_TIMEOUT": "niedostępny",
"GPX_FILE": "Plik GPX",
"DISTANCE": "Dystans",
"DURATION": "Czas",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Twoja trasa została wyznaczona",
"NO_ROUTE_FOUND": "Nie można wyznaczyć trasy",
//printing
"OVERVIEW_MAP": "Mapa poglądowa",
// directions
"N": "północ",
"E": "wschód",
"S": "południe",
"W": "zachód",
"NE": "północny wschód",
"SE": "południowy wschód",
"SW": "południowy zachód",
"NW": "połnocny zachód",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Nieznana instrukcja[ na <b>%s</b>]",
"DIRECTION_1":"Kontynuuj[ drogą <b>%s</b>]",
"DIRECTION_2":"Skręć lekko w prawo[ na drogę <b>%s</b>]",
"DIRECTION_3":"Skręć w prawo[ na drogę <b>%s</b>]",
"DIRECTION_4":"Skręć ostro w prawo[ na drogę <b>%s</b>]",
"DIRECTION_5":"Zawróć[ na drogę <b>%s</b>]",
"DIRECTION_6":"Skręć ostro w lewo[ na drogę <b>%s</b>]",
"DIRECTION_7":"Skręć w lewo[ na drogę <b>%s</b>]",
"DIRECTION_8":"Skręć lekko w lewo[ na drogę <b>%s</b>]",
"DIRECTION_10":"Podążaj na <b>%d</b>[ drogą <b>%s</b>]",
"DIRECTION_11-1":"Wjedź na rondo, zjedź pierwszym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-2":"Wjedź na rondo, zjedź drugim zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-3":"Wjedź na rondo, zjedź trzecim zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-4":"Wjedź na rondo, zjedź czwartym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-5":"Wjedź na rondo, zjedź piątym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-6":"Wjedź na rondo, zjedź szóstym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-7":"Wjedź na rondo, zjedź siódmym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-8":"Wjedź na rondo, zjedź ósmym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-9":"Wjedź na rondo, zjedź dziewiątym zjazdem[ na drogę <b>%s</b>]",
"DIRECTION_11-x":"Wjedź na rondo, zjedź wybranym przez siebie zjazdem [ na drogę <b>%s</b>]",
"DIRECTION_15":"Cel został osiągnięty"
};

//set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("pl");
