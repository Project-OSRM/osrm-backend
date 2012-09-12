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


OSRM.Localization["tr"] = {
// own language
"CULTURE": "tr-TR",
"LANGUAGE": "Türkçe",
// gui
"GUI_START": "Başlangıç",
"GUI_END": "Bitiş",
"GUI_RESET": "Reset",
"GUI_ZOOM_ON_ROUTE": "Güzergah Üzerinde Yakınlaştırma",
"GUI_ZOOM_ON_USER": "Kullanıcının Yakınlaştırma",
"GUI_SEARCH": "Göstermek",
"GUI_REVERSE": "Çevir",
"GUI_START_TOOLTIP": "Nereden",
"GUI_END_TOOLTIP": "Nereye",
"GUI_MAIN_WINDOW": "Ana Pencere",
"GUI_ZOOM_IN": "büyütmek",
"GUI_ZOOM_OUT": "azaltmak",
// config
"GUI_CONFIGURATION": "Konfigürasyon",
"GUI_LANGUAGE": "Dil",
"GUI_UNITS": "Units",
"GUI_KILOMETERS": "Kilometre",
"GUI_MILES": "Mil",
// mapping
"GUI_MAPPING_TOOLS": "Harita Araçları",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Önemli İsimlendirilmemiş Sokaklar",
"GUI_SHOW_PREVIOUS_ROUTES": "Önceki Rotaları Göster",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Sonuçları Göster",
"FOUND_X_RESULTS": "found %i results",
"TIMED_OUT": "Timed Out",
"NO_RESULTS_FOUND": "Hiçbir Sonuç Bulunamadı",
"NO_RESULTS_FOUND_SOURCE": "Başlangıç için hiç sonuç bulunmadı",
"NO_RESULTS_FOUND_TARGET": "Bitiş için hiç sonuç bulunamadı",
// routing
"ROUTE_DESCRIPTION": "Rota Açıklaması",
"GET_LINK_TO_ROUTE": "Link Oluştur",
"GENERATE_LINK_TO_ROUTE": "Link bekleniyor",
"LINK_TO_ROUTE_TIMEOUT": "uygun değil",
"GPX_FILE": "GPX Dosyası",
"DISTANCE": "Mesafe",
"DURATION": "Süre",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Rotanız Hesaplandı",
"NO_ROUTE_FOUND": "Hiç uygun rota yok",
// printing
"OVERVIEW_MAP": "Haritaya genel bakış",
"NO_ROUTE_SELECTED": "Hiç rota seçilmedi",
// routing engines
"ENGINE_0": "Araba (hızlı)",
// directions
"N": "north",
"E": "east",
"S": "south",
"W": "west",
"NE": "northeast",
"SE": "southeast",
"SW": "southwest",
"NW": "northwest",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Unknown instruction[ onto <b>%s</b>]",
"DIRECTION_1":"Continue[ onto <b>%s</b>]",
"DIRECTION_2":"Turn slight right[ onto <b>%s</b>]",
"DIRECTION_3":"Turn right[ onto <b>%s</b>]",
"DIRECTION_4":"Turn sharp right[ onto <b>%s</b>]",
"DIRECTION_5":"U-Turn[ onto <b>%s</b>]",
"DIRECTION_6":"Turn sharp left[ onto <b>%s</b>]",
"DIRECTION_7":"Turn left[ onto <b>%s</b>]",
"DIRECTION_8":"Turn slight left[ onto <b>%s</b>]",
"DIRECTION_10":"Head <b>%d</b>[ onto <b>%s</b>]",
"DIRECTION_11-1":"Enter roundabout and leave at first exit[ onto <b>%s</b>]",
"DIRECTION_11-2":"Enter roundabout and leave at second exit[ onto <b>%s</b>]",
"DIRECTION_11-3":"Enter roundabout and leave at third exit[ onto <b>%s</b>]",
"DIRECTION_11-4":"Enter roundabout and leave at fourth exit[ onto <b>%s</b>]",
"DIRECTION_11-5":"Enter roundabout and leave at fifth exit[ onto <b>%s</b>]",
"DIRECTION_11-6":"Enter roundabout and leave at sixth exit[ onto <b>%s</b>]",
"DIRECTION_11-7":"Enter roundabout and leave at seventh exit[ onto <b>%s</b>]",
"DIRECTION_11-8":"Enter roundabout and leave at eighth exit[ onto <b>%s</b>]",
"DIRECTION_11-9":"Enter roundabout and leave at nineth exit[ onto <b>%s</b>]",
"DIRECTION_11-x":"Enter roundabout and leave at one of the too many exits[ onto <b>%s</b>]",
"DIRECTION_15":"Hedefinize ulaştınız"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING==true )
	OSRM.Localization.setLanguage("tr");
