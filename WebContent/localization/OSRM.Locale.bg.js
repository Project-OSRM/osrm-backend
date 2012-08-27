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
// [Bulgarian language support]


OSRM.Localization["bg"] = {
// own language
"CULTURE": "bg-BG",
"LANGUAGE": "Български",
// gui
"GUI_START": "Начало",
"GUI_END": "Край",
"GUI_RESET": "Изчисти",
"GUI_ZOOM_ON_ROUTE": "Приближи на маршрута",
"GUI_ZOOM_ON_USER": "Приближи на потребител",
"GUI_SEARCH": "Покажи",
"GUI_REVERSE": "Размени",
"GUI_START_TOOLTIP": "Въведи начало",
"GUI_END_TOOLTIP": "Въведи карйна цел",
"GUI_MAIN_WINDOW": "главния прозорец",
"GUI_ZOOM_IN": "Приближаване",
"GUI_ZOOM_OUT": "Oтдалечаване",
// config
"GUI_CONFIGURATION": "Конфигурация",
"GUI_LANGUAGE": "Език",
"GUI_UNITS": "единици",
"GUI_KILOMETERS": "Километри",
"GUI_MILES": "Мили",
// mapping
"GUI_MAPPING_TOOLS": "Инструменти за карта",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Подчертай неименувани улици",
"GUI_SHOW_PREVIOUS_ROUTES": "Покажи преднишни маршрути",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Грешки",
// geocoder
"SEARCH_RESULTS": "Резултати от търсене",
"FOUND_X_RESULTS": "намерени %i резултати",
"TIMED_OUT": "Прекъсване",
"NO_RESULTS_FOUND": "Няма резултати",
"NO_RESULTS_FOUND_SOURCE": "Няма резултати за начало",
"NO_RESULTS_FOUND_TARGET": "Няма резултати за край",
// routing
"ROUTE_DESCRIPTION": "Описание на маршрута",
"GET_LINK_TO_ROUTE": "Генерирай препратка",
"GENERATE_LINK_TO_ROUTE": "изчакване за препратка",
"LINK_TO_ROUTE_TIMEOUT": "не е наличен",
"GPX_FILE": "GPX файл",
"DISTANCE": "Разстояние",
"DURATION": "Продължителност",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Маршрутът се изчислява",
"NO_ROUTE_FOUND": "Не е възможен маршрут",
// printing
"OVERVIEW_MAP": "сбит изглед",
"NO_ROUTE_SELECTED": "Не е изберан маршрут",
// routing engines
"ENGINE_0": "Kола (най-бързо)",
// directions
"N": "север",
"E": "изток",
"S": "юг",
"W": "запад",
"NE": "североизток",
"SE": "югоизток",
"SW": "югозапад",
"NW": "североизток",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Невалидна инструкция[ по <b>%s</b>]",
"DIRECTION_1":"Продължи[ по <b>%s</b>]",
"DIRECTION_2":"Завий леко вдясно[ по <b>%s</b>]",
"DIRECTION_3":"Завий вдясно[ по <b>%s</b>]",
"DIRECTION_4":"Завий остро вдясно[ по <b>%s</b>]",
"DIRECTION_5":"Обратен завой[ по <b>%s</b>]",
"DIRECTION_6":"Завий остро вляво[ по <b>%s</b>]",
"DIRECTION_7":"Завий вляво[ по <b>%s</b>]",
"DIRECTION_8":"Завий леко вляво[ по <b>%s</b>]",
"DIRECTION_10":"Направо <b>%d</b>[ по <b>%s</b>]",
"DIRECTION_11-1":"Влез в кръговото и излез на първия изход[ по <b>%s</b>]",
"DIRECTION_11-2":"Влез в кръговото и излез на втория изход[ по <b>%s</b>]",
"DIRECTION_11-3":"Влез в кръговото и излез на третия изход[ по <b>%s</b>]",
"DIRECTION_11-4":"Влез в кръговото и излез на четвъртия изход[ по <b>%s</b>]",
"DIRECTION_11-5":"Влез в кръговото и излез на петия изход[ по <b>%s</b>]",
"DIRECTION_11-6":"Влез в кръговото и излез на шестия изход[ по <b>%s</b>]",
"DIRECTION_11-7":"Влез в кръговото и излез на седмия изход[ по <b>%s</b>]",
"DIRECTION_11-8":"Влез в кръговото и излез на осмия изход[ по <b>%s</b>]",
"DIRECTION_11-9":"Влез в кръговото и излез на деветия изход[ по <b>%s</b>]",
"DIRECTION_11-x":"Влез в кръговото и излез на един от многото изходи[ по <b>%s</b>]",
"DIRECTION_15":"Достигна крайната цел"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("bg");
