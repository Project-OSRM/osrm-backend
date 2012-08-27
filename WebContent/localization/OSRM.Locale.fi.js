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
// [Finnish language support]


OSRM.Localization["fi"] = {
// own language
"CULTURE": "fi-FI",
"LANGUAGE": "Suomi",
// gui
"GUI_START": "Lähtöpaikka",
"GUI_END": "Määränpää",
"GUI_RESET": "Tyhjennä",
"GUI_ZOOM_ON_ROUTE": "Zoom reitillä",
"GUI_ZOOM_ON_USER": "Zoom käyttäjä",
"GUI_SEARCH": "Etsi",
"GUI_REVERSE": "Käänteinen&nbsp;reitti",
"GUI_START_TOOLTIP": "Syötä lähtöpaikka",
"GUI_END_TOOLTIP": "Syötä määränpää",
"GUI_MAIN_WINDOW": "Pääikkuna",
"GUI_ZOOM_IN": "Lähennä",
"GUI_ZOOM_OUT": "Loitonna",
// config
"GUI_CONFIGURATION": "Kokoonpano",
"GUI_LANGUAGE": "Kieli",
"GUI_UNITS": "Yksiköt",
"GUI_KILOMETERS": "Kilometri",
"GUI_MILES": "Miles",
// mapping
"GUI_MAPPING_TOOLS": "Kartoitustyökalut",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Korosta nimettömät tiet",
"GUI_SHOW_PREVIOUS_ROUTES": "Näytä edelliset reitit",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Haun tulokset",
"FOUND_X_RESULTS": "Löytyi %i vaihtoehtoa",
"TIMED_OUT": "Aikakatkaisu",
"NO_RESULTS_FOUND": "Ei hakutuloksia",
"NO_RESULTS_FOUND_SOURCE": "Ei hakutuloksia lähtöpaikka",
"NO_RESULTS_FOUND_TARGET": "Ei hakutuloksia määränpäälle",
// routing
"ROUTE_DESCRIPTION": "Reittiohjeet",
"GET_LINK_TO_ROUTE": "Luo linkki",
"GENERATE_LINK_TO_ROUTE": "odotetaan linkkiä",
"LINK_TO_ROUTE_TIMEOUT": "ei saatavilla",
"GPX_FILE": "GPX-tiedosto",
"DISTANCE": "Etäisyys",
"DURATION": "Aika",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Reittiä lasketaan",
"NO_ROUTE_FOUND": "Reittiä ei löytynyt",
// printing
"OVERVIEW_MAP": "Yleiskuvakartta",
"NO_ROUTE_SELECTED": "Ei reitti valittu",
// routing engines
"ENGINE_0": "Auton (nopein)",
// directions
"N": "pohjoiseen",
"E": "itään",
"S": "etelään",
"W": "länteen",
"NE": "koilliseen",
"SE": "kaakkoon",
"SW": "lounaaseen",
"NW": "luoteeseen",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Tuntematon ohje[ tielle <b>%s</b>]",
"DIRECTION_1":"Jatka[ tielle <b>%s</b>]",
"DIRECTION_2":"Käänny loivasti oikealle[ tielle <b>%s</b>]",
"DIRECTION_3":"Käänny oikealle[ tielle <b>%s</b>]",
"DIRECTION_4":"Käänny jyrkästi oikealle[ tielle <b>%s</b>]",
"DIRECTION_5":"Tee U-käännös[ tiellä <b>%s</b>]",
"DIRECTION_6":"Käänny jyrkästi vasemmalle [ tielle <b>%s</b>]",
"DIRECTION_7":"Käänny vasemmalle[ tielle <b>%s</b>]",
"DIRECTION_8":"Käänny loivasti vasemmalle[ tielle <b>%s</b>]",
"DIRECTION_10":"Aja <b>%d</b> [ tielle <b>%s</b>]",
"DIRECTION_11-1":"Aja liikenneympyrään ja poistu ensimmäisestä haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-2":"Aja liikenneympyrään ja poistu toisesta haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-3":"Aja liikenneympyrään ja poistu kolmannesta haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-4":"Aja liikenneympyrään ja poistu neljännestä haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-5":"Aja liikenneympyrään ja poistu viidennestä haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-6":"Aja liikenneympyrään ja poistu kuudennesta haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-7":"Aja liikenneympyrään ja poistu seitsemännestä haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-8":"Aja liikenneympyrään ja poistu kahdeksannesta haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-9":"Aja liikenneympyrään ja poistu yhdeksännestä haarasta[ tielle <b>%s</b>]",
"DIRECTION_11-x":"Aja liikenneympyrään ja poistu monen haaran jälkeen[ tielle <b>%s</b>]",
"DIRECTION_15":"Saavuit määränpäähän"
};

// set GUI language tielle load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("fi");
