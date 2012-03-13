// localization

OSRM.Localization = {
language: "en",

translate: function(text) {
	if( OSRM.Localization[OSRM.Localization.language][text] )
		return OSRM.Localization[OSRM.Localization.language][text];
	else if( OSRM.Localization[OSRM.Localization.language][text] )
		return OSRM.Localization[OSRM.Localization.language][text];
	else
		return text;
},
};
OSRM.loc = OSRM.Localization.translate;

OSRM.Localization["de"] = {
//gui
"GUI_START": "Start",
"GUI_END": "Ende",
"GUI_RESET": "Reset",
"GUI_SEARCH": "Suchen",
"GUI_ROUTE": "Route",
"GUI_REVERSE": "Umdrehen",
"GUI_OPTIONS": "Optionen",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Unbenannte Stra�en hervorheben",
"GUI_START_TOOLTIP": "Startposition eingeben",
"GUI_END_TOOLTIP": "Zielposition eingeben",
"GUI_LEGAL_NOTICE": "GUI2 v0.1 120313 - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.mapquest.com/'>MapQuest</a>",
// geocoder
"SEARCH_RESULTS": "Suchergebnisse",
"TIMED_OUT": "Zeit�berschreitung",
"NO_RESULTS_FOUND": "Keine Ergebnisse gefunden",
// routing
"ROUTE_DESCRIPTION": "Routenbeschreibung",
"GET_LINK": "Generiere Link",
"LINK_TO_ROUTE": "Link zur Route",
"LINK_TO_ROUTE_TIMEOUT": "nicht möglich",
"GPX_FILE": "GPX Datei",
"DISTANCE": "Distanz",
"DURATION": "Dauer",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Ihre Route wird berechnet",
"NO_ROUTE_FOUND": "Keine Route hierher m�glich",
};

OSRM.Localization["en"] = {
//gui
"GUI_START": "Start",
"GUI_END": "End",
"GUI_RESET": "Reset",
"GUI_SEARCH": "Search",
"GUI_ROUTE": "Route",
"GUI_REVERSE": "Reverse",
"GUI_OPTIONS": "Options",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Highlight unnamed streets",
"GUI_START_TOOLTIP": "Enter start",
"GUI_END_TOOLTIP": "Enter destination",
"GUI_LEGAL_NOTICE": "GUI2 v0.1 120313 - OSRM hosting by <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder by <a href='http://www.mapquest.com/'>MapQuest</a>",
// geocoder
"SEARCH_RESULTS": "Search Results",
"TIMED_OUT": "Timed Out",
"NO_RESULTS_FOUND": "No results found",
//routing
"ROUTE_DESCRIPTION": "Route Description",
"GET_LINK": "Generate Link",
"LINK_TO_ROUTE": "Route Link",
"LINK_TO_ROUTE_TIMEOUT": "not available",
"GPX_FILE": "GPX File",
"DISTANCE": "Distance",
"DURATION": "Duration",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Your route is being computed",
"NO_ROUTE_FOUND": "No route possible",
};