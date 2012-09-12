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
// [Japanese language support]


OSRM.Localization["ja"] = {
// own language
"CULTURE": "ja-JP",
"LANGUAGE": "日本人",
// gui
"GUI_START": "開&nbsp;始",
"GUI_END": "終&nbsp;了",
"GUI_RESET": "リセット",
"GUI_ZOOM_ON_ROUTE": "ルートにズーム",
"GUI_ZOOM_ON_USER": "ユーザーへのズーム",
"GUI_SEARCH": "表&nbsp;示",
"GUI_REVERSE": "逆にする",
"GUI_START_TOOLTIP": "出発地を入力",
"GUI_END_TOOLTIP": "目的地を入力",
"GUI_MAIN_WINDOW": "メインウインドウ",
"GUI_ZOOM_IN": "拡大する",
"GUI_ZOOM_OUT": "減らす",
// config
"GUI_CONFIGURATION": "設定",
"GUI_LANGUAGE": "言語",
"GUI_UNITS": "単位",
"GUI_KILOMETERS": "キロメートル",
"GUI_MILES": "マイル",
// mapping
"GUI_MAPPING_TOOLS": "マッピングツール",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "名前の無い道路をハイライト",
"GUI_SHOW_PREVIOUS_ROUTES": "以前のルートを表示",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM バグ",
// geocoder
"SEARCH_RESULTS": "検索結果",
"FOUND_X_RESULTS": "%i 件が見つかりました",
"TIMED_OUT": "タイムアウト",
"NO_RESULTS_FOUND": "見つかりませんでした",
"NO_RESULTS_FOUND_SOURCE": "出発地が見つかりませんでした",
"NO_RESULTS_FOUND_TARGET": "目的地見つかりませんでした",
// routing
"ROUTE_DESCRIPTION": "ルートの説明",
"GET_LINK_TO_ROUTE": "リンクを生成",
"GENERATE_LINK_TO_ROUTE": "リンクを作成",
"LINK_TO_ROUTE_TIMEOUT": "利用できません",
"GPX_FILE": "GPX ファイル",
"DISTANCE": "距&nbsp;離",
"DURATION": "期&nbsp;間",
"YOUR_ROUTE_IS_BEING_COMPUTED": "ルートを計算しています",
"NO_ROUTE_FOUND": "ルートが見つかりませんでした",
// printing
"OVERVIEW_MAP": "概観図",
"NO_ROUTE_SELECTED": "ルートが選択されていません",
//routing engines
"ENGINE_0": "カー（最速）",
// directionsVergrößern
"N": "北",
"E": "東",
"S": "南",
"W": "西",
"NE": "北東",
"SE": "南東",
"SW": "南西",
"NW": "北西",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"[<b>%s</b> に]不明な指示",
"DIRECTION_1":"[<b>%s</b> 上を]進む",
"DIRECTION_2":"[<b>%s</b> 上を]右に緩く曲がる",
"DIRECTION_3":"[<b>%s</b> 上を]右に曲がる",
"DIRECTION_4":"[<b>%s</b> 上を]右に鋭く曲がる",
"DIRECTION_5":"[<b>%s</b> 上を]Uターンする",
"DIRECTION_6":"[<b>%s</b> 上を]左に鋭く曲がる",
"DIRECTION_7":"[<b>%s</b> 上を]左に曲がる",
"DIRECTION_8":"[<b>%s</b> 上を]左に緩く曲がる",
"DIRECTION_10":"[<b>%s</b> 上の] <b>%d</b> に向かう",
"DIRECTION_11-1":"[<b>%s</b> 上の]円形交差点に入り、1つ目の出口で出る",
"DIRECTION_11-2":"[<b>%s</b> 上の]円形交差点に入り、2つ目の出口で出る",
"DIRECTION_11-3":"[<b>%s</b> 上の]円形交差点に入り、3つ目の出口で出る",
"DIRECTION_11-4":"[<b>%s</b> 上の]円形交差点に入り、4つ目の出口で出る",
"DIRECTION_11-5":"[<b>%s</b> 上の]円形交差点に入り、5つ目の出口で出る",
"DIRECTION_11-6":"[<b>%s</b> 上の]円形交差点に入り、6つ目の出口で出る",
"DIRECTION_11-7":"[<b>%s</b> 上の]円形交差点に入り、7つ目の出口で出る",
"DIRECTION_11-8":"[<b>%s</b> 上の]円形交差点に入り、8つ目の出口で出る",
"DIRECTION_11-9":"[<b>%s</b> 上の]円形交差点に入り、9つ目の出口で出る",
"DIRECTION_11-x":"[<b>%s</b> 上の]円形交差点に入り、多くの出口のいずれかで出る",
"DIRECTION_15":"目的地に到着しました"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("ja");
