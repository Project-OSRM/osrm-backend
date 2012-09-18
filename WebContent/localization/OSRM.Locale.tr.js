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
// [Turkish language support]


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
"ENGINE_0": "Araba (en hızlı)",
// directions
"N": "kuzey",
"E": "doğu",
"S": "güney",
"W": "batı",
"NE": "kuzeydoğu",
"SE": "güneydoğu",
"SW": "güneybatı",
"NW": "kuzeybatı",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Bilinmeyen açıklama[ onto <b>%s</b>]",
"DIRECTION_1":"Devam[ onto <b>%s</b>]",
"DIRECTION_2":"Hafif sağa dönün[ onto <b>%s</b>]",
"DIRECTION_3":"Sağa dönün[ onto <b>%s</b>]",
"DIRECTION_4":"Sağa keskin dönün[ onto <b>%s</b>]",
"DIRECTION_5":"U-Dönüşü[ onto <b>%s</b>]",
"DIRECTION_6":"Sola keskin dönün[ onto <b>%s</b>]",
"DIRECTION_7":"Sola dönün[ onto <b>%s</b>]",
"DIRECTION_8":"Hafif sola dönün[ onto <b>%s</b>]",
"DIRECTION_10":"Yön <b>%d</b>[ onto <b>%s</b>]",
"DIRECTION_11-1":"Kavşağa girin ve ilk çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-2":"Kavşağa girin ve ikinci çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-3":"Kavşağa girin ve üçüncü çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-4":"Kavşağa girin ve dördüncü çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-5":"Kavşağa girin ve beşinci çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-6":"Kavşağa girin ve altıncı çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-7":"Kavşağa girin ve yedinci çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-8":"Kavşağa girin ve sekizinci çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-9":"Kavşağa girin ve dokuzuncu çıkıştan çıkın[ onto <b>%s</b>]",
"DIRECTION_11-x":"Kavşağa girin ve birçok çıkışın birinden çıkın[ onto <b>%s</b>]",
"DIRECTION_15":"Hedefinize ulaştınız",
// notifications
"NOTIFICATION_MAINTENANCE_HEADER":	"Programlı Bakım",
"NOTIFICATION_MAINTENANCE_BODY":	"OSRM Web sayfası planlı bakımla yazılmıştır. " + 
									"Lütfen güncellenmeler yapılırken sabırlı olun. " +
									"Site kısa bir süre sonra çevrimiçi olacaktır." +
									"<br/><br/>" + 
									"Aynı zamanda dışarıya çıkıp komşunuzu haritalamayı isteyebilirsiniz..." + 
									"<br/><br/><br/>[OSRM]",
"NOTIFICATION_LOCALIZATION_HEADER":	"Biliyor muydunuz? Dil görünümünü değiştirebilirsiniz.",
"NOTIFICATION_LOCALIZATION_BODY":	"Favori dilinizi seçmek için üst sol köşedeki aşağı açılna menüye tıklayabilirsiniz. " +
									"<br/><br/>" +
									"İstediğiniz dili bulamazsanız umutsuzluğa düşmeyin. " +
									"Eğer isterseniz, fazladan çeviriler elde etmek için yardım edebilirsiniz! " +
									"Ziyaret edin <a href='https://github.com/DennisSchiefer/Project-OSRM-Web'>here</a> daha fazla bilgi için.",
"NOTIFICATION_CLICKING_HEADER":		"Biliyor muydunuz? Rota işaretleyicilerini ayarlamak için haritaya tıklayabilirsiniz.",
"NOTIFICATION_CLICKING_BODY":		"İşaretleyici kaynağını (yeşil) yada hedef işaretleyici (kırmızı)ayarlamak için farenin sol tarafıyla haritanın üzerine tıklayabilirsiniz. " +
									"Eğer kaynak işaretleyici zaten varsa. " +
									"Seçilen bölgenin adresi sola doğru kutularda gözükecek. " + 
									"<br/><br/>" +
									"Farenin sol tarafına tıklayarak işaretleyiciyi silebilirsiniz.",
"NOTIFICATION_DRAGGING_HEADER":		"Biliyor muydunuz? Rota işaretleyicisini harita üzerinde sürükleyebilirsiniz.",
"NOTIFICATION_DRAGGING_BODY":		"İşaretleyiciyi farenin sol tarafına tıklayarak veya basılı tutarak sürükleyebilirsiniz. " +
									"Sonra işaretleyiciyi haritanın çevresinde hareket ettirebilirsiniz ve rota kendini otomatik olarak yeniler. " +
									"<br/><br/>" +
									"Asıl rotalarına sürükleyerek orta dereceli bir işaretleyici oluşturabilirsiniz ! "
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING==true )
	OSRM.Localization.setLanguage("tr");
