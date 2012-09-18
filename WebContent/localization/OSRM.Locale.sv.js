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
// [Swedish language support]


OSRM.Localization["sv"] = {
// own language
"CULTURE": "sv-SE",
"LANGUAGE": "Svenska",
// gui
"GUI_START": "Start",
"GUI_END": "Mål",
"GUI_RESET": "Nollställ",
"GUI_ZOOM_ON_ROUTE": "Zooma in rutt",
"GUI_ZOOM_ON_USER": "Zooma in användare",
"GUI_SEARCH": "Sök",
"GUI_REVERSE": "Omvänt",
"GUI_START_TOOLTIP": "Lägg in start",
"GUI_END_TOOLTIP": "Lägg in destination",
"GUI_MAIN_WINDOW": "Huvudfönstret",
"GUI_ZOOM_IN": "Zooma in",
"GUI_ZOOM_OUT": "Zooma ut",
// config
"GUI_CONFIGURATION": "Konfiguration",
"GUI_LANGUAGE": "Språk",
"GUI_UNITS": "Enheter",
"GUI_KILOMETERS": "Kilometer",
"GUI_MILES": "Miles",
// mapping
"GUI_MAPPING_TOOLS": "Kortläggnings verktyg",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Framhäv icke namngedda vägar",
"GUI_SHOW_PREVIOUS_ROUTES": "Visa tidigare rutter",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Sökresultat",
"FOUND_X_RESULTS": "fann %i resultat",
"TIMED_OUT": "Inget svar",
"NO_RESULTS_FOUND": "Inga resultat",
"NO_RESULTS_FOUND_SOURCE": "Inga resultat för start",
"NO_RESULTS_FOUND_TARGET": "Inga resultat för destination",
// routing
"ROUTE_DESCRIPTION": "Ruttbeskrivelse",
"GET_LINK_TO_ROUTE": "Skapa länk",
"GENERATE_LINK_TO_ROUTE": "väntar på länk",
"LINK_TO_ROUTE_TIMEOUT": "icke tillgänglig",
"GPX_FILE": "GPX Fil",
"DISTANCE": "Distans",
"DURATION": "Tid",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Din rutt beräknas",
"NO_ROUTE_FOUND": "Ingen möjlig rutt funnen",
// printing
"OVERVIEW_MAP": "Översiktskarta",
"NO_ROUTE_SELECTED": "Ingen rutt vald",
// routing engines
"ENGINE_0": "Bil (snabbaste)",
// directions
"N": "nord",
"E": "öst",
"S": "syd",
"W": "väst",
"NE": "nordost",
"SE": "sydost",
"SW": "sydväst",
"NW": "nordväst",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Okänd instruktion[ på <b>%s</b>]",
"DIRECTION_1":"Fortsätt[ på <b>%s</b>]",
"DIRECTION_2":"Sväng svagt till höger[ in på <b>%s</b>]",
"DIRECTION_3":"Sväng till höger[ in på <b>%s</b>]",
"DIRECTION_4":"Sväng skarpt till höger[ in på <b>%s</b>]",
"DIRECTION_5":"U-sväng[ på <b>%s</b>]",
"DIRECTION_6":"Sväng skarpt till vänster[ in på <b>%s</b>]",
"DIRECTION_7":"Sväng till vänster[ in på <b>%s</b>]",
"DIRECTION_8":"Sväng svagt till vänster[ in på <b>%s</b>]",
"DIRECTION_10":"Kör mot <b>%d</b>[ på <b>%s</b>]",
"DIRECTION_11-1":"Kör in i rondellen och tag första avfarten[ in på <b>%s</b>]",
"DIRECTION_11-2":"Kör in i rondellen och tag andra avfarten[ in på <b>%s</b>]",
"DIRECTION_11-3":"Kör in i rondellen och tag tredje avfarten[ in på <b>%s</b>]",
"DIRECTION_11-4":"Kör in i rondellen och tag fjärde avfarten[ in på <b>%s</b>]",
"DIRECTION_11-5":"Kör in i rondellen och tag femte avfarten[ in på <b>%s</b>]",
"DIRECTION_11-6":"Kör in i rondellen och tag sjätte avfarten[ in på <b>%s</b>]",
"DIRECTION_11-7":"Kör in i rondellen och tag sjunde avfarten[ in på <b>%s</b>]",
"DIRECTION_11-8":"Kör in i rondellen och tag åttonde avfarten[ in på <b>%s</b>]",
"DIRECTION_11-9":"Kör in i rondellen och tag nionde avfarten[ in på <b>%s</b>]",
"DIRECTION_11-x":"Kör in i rondellen och tag en av de allt för många avfarterna[ in på <b>%s</b>]",
"DIRECTION_15":"Du är framme",
// notifications
"NOTIFICATION_MAINTENANCE_HEADER":	"Schemalagt underhåll",
"NOTIFICATION_MAINTENANCE_BODY":	"OSRM Websida ligger nere pga. schemalagt underhåll. " + 
									"Var god djöj till de nödvändiga uppdateringarna är slutförda. " +
									"Sidan kommer vara upp snart." +
									"<br/><br/>" + 
									"Under tiden kanske du vill kartlägga ditt närområde..." + 
									"<br/><br/><br/>[OSRM]",
"NOTIFICATION_LOCALIZATION_HEADER":	"Visste du att? Du kan ändra språk.",
"NOTIFICATION_LOCALIZATION_BODY":	"Du kan använda menyn uppe till vänster för att välja ditt språk. " +
									"<br/><br/>" +
									"Frukta inte, om ditt språk inte finns. " +
									"Du kan då hjälpa till och översätta OSRM till ditt språk! " +
									"Visit <a href='https://github.com/DennisSchiefer/Project-OSRM-Web'>here</a> for more information.",
"NOTIFICATION_CLICKING_HEADER":		"Visste du att? Du kan klicka på kartan för att lägga till ruttmarkörer.",
"NOTIFICATION_CLICKING_BODY":		"Du kan klicka på kartan med vänster musknapp för att sätta startpunkt (grön) eller målpunkt (röd), " +
									"om det redan finns en startpunkt. " +
									"Addressen för den tillagda punkten kommer finnas i sökfälter till vänster. " + 
									"<br/><br/>" +
									"Du kan ta bort en markör genom att återigen klicka på den.",
"NOTIFICATION_DRAGGING_HEADER":		"Visste du att? Du kan dra i varje ruttmarkör på kartan.",
"NOTIFICATION_DRAGGING_BODY":		"Du kan dra i varje ruttmarkör genom att klicka och hålla nere vänster musknapp på den och sedan dra densamma. " +
									"Då kan du flytta ruttmarkören på kartan och rutten uppdateras automatiskt. " +
									"<br/><br/>" +
									"Du kan även skappa mellanpunktsmarkörer genom att dra någonstans på rutten! "
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("sv");
