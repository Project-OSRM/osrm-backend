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


OSRM.Localization["ru"] = {
// own language
"CULTURE": "ru-RU",
"LANGUAGE": "Русский",
// gui
"GUI_START": "Начало",
"GUI_END": "Конец",
"GUI_RESET": "Сброс",
"GUI_ZOOM_ON_ROUTE": "зум на маршрут",
"GUI_ZOOM_ON_USER": "зум на Пользователь",
"GUI_SEARCH": "Показать",
"GUI_REVERSE": "Обратно",
"GUI_START_TOOLTIP": "Укажите начальную точку",
"GUI_END_TOOLTIP": "Укажите пункт назначения",
"GUI_MAIN_WINDOW": "Главное окно",
"GUI_ZOOM_IN": "Yвеличить",
"GUI_ZOOM_OUT": "Mасштаб",
// config
"GUI_CONFIGURATION": "Настройки",
"GUI_LANGUAGE": "Язык",
"GUI_UNITS": "Единицы",
"GUI_KILOMETERS": "Километры",
"GUI_MILES": "Мили",
// mapping
"GUI_MAPPING_TOOLS": "Настройки карты",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Выделить безымянные улицы",
"GUI_SHOW_PREVIOUS_ROUTES": "Отображать предыдущий маршрут",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Результаты поиска",
"FOUND_X_RESULTS": "найдено %i результатов",
"TIMED_OUT": "Превышен интервал ожидания",
"NO_RESULTS_FOUND": "Ничего не найдено",
"NO_RESULTS_FOUND_SOURCE": "Начальная точка не найдена",
"NO_RESULTS_FOUND_TARGET": "Пункт назначения не найден",
// routing
"ROUTE_DESCRIPTION": "Описание маршрута",
"GET_LINK_TO_ROUTE": "Постоянная ссылка",
"GENERATE_LINK_TO_ROUTE": "создание ссылки",
"LINK_TO_ROUTE_TIMEOUT": "недоступно",
"GPX_FILE": "GPX Файл",
"DISTANCE": "Расстояние",
"DURATION": "Время",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Вычисление маршрута",
"NO_ROUTE_FOUND": "Маршрут не возможен",
// printing
"OVERVIEW_MAP": "Обзорная карта",
"NO_ROUTE_SELECTED": "Маршрут не выбран",
// routing engines
"ENGINE_0": "Aвтомобиля (быстрый)",
// directions
"N": "север",
"E": "восток",
"S": "юг",
"W": "запад",
"NE": "северо-восток",
"SE": "юго-восток",
"SW": "юго-запад",
"NW": "северо-запад",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Неизвестная инструкция[ по <b>%s</b>]",
"DIRECTION_1":"Продолжайте движение[ по <b>%s</b>]",
"DIRECTION_2":"Примите вправо[ на <b>%s</b>]",
"DIRECTION_3":"Поверните направо[ на <b>%s</b>]",
"DIRECTION_4":"Поверните резко направо[ на <b>%s</b>]",
"DIRECTION_5":"U-образный разворот[ на <b>%s</b>]",
"DIRECTION_6":"Поверните резко налево[ на <b>%s</b>]",
"DIRECTION_7":"Поверните налево[ на <b>%s</b>]",
"DIRECTION_8":"Примите влево[ на <b>%s</b>]",
"DIRECTION_10":"Направляйтесь на <b>%d</b>[ по <b>%s</b>]",
"DIRECTION_11-1":"На кольцевой дороге выполните 1-ый съезд[ на <b>%s</b>]",
"DIRECTION_11-2":"На кольцевой дороге выполните 2-ой съезд[ на <b>%s</b>]",
"DIRECTION_11-3":"На кольцевой дороге выполните 3-ий съезд[ на <b>%s</b>]",
"DIRECTION_11-4":"На кольцевой дороге выполните 4-ый съезд[ на <b>%s</b>]",
"DIRECTION_11-5":"На кольцевой дороге выполните 5-ый съезд[ на <b>%s</b>]",
"DIRECTION_11-6":"На кольцевой дороге выполните 6-ой съезд[ на <b>%s</b>]",
"DIRECTION_11-7":"На кольцевой дороге выполните 7-ой съезд[ на <b>%s</b>]",
"DIRECTION_11-8":"На кольцевой дороге выполните 8-ой съезд[ на <b>%s</b>]",
"DIRECTION_11-9":"На кольцевой дороге выполните 9-ый съезд[ на <b>%s</b>]",
"DIRECTION_11-x":"На кольцевой дороге выполните съезд[ на <b>%s</b>]",
"DIRECTION_15":"Вы прибыли в пункт назначения"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("ru");
