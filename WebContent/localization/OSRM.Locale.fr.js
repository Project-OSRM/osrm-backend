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
// [French language support]


OSRM.Localization["fr"] = {
// own language
"CULTURE": "fr-FR",
"LANGUAGE": "Français",
//gui
"GUI_START": "Départ",
"GUI_END": "Arrivée",
"GUI_RESET": "Réinitialiser",
"GUI_ZOOM_ON_ROUTE": "Zoom sur la Route",
"GUI_ZOOM_ON_USER": "Zoom sur le Utilisateur",
"GUI_SEARCH": "Montrer",
"GUI_REVERSE": "Inverser",
"GUI_START_TOOLTIP": "Entrez le lieu de départ",
"GUI_END_TOOLTIP": "Entrez le lieu d’arrivée",
"GUI_MAIN_WINDOW": "Fenêtre principale",
"GUI_ZOOM_IN": "Zoomer",
"GUI_ZOOM_OUT": "Rétrécir",
// config
"GUI_CONFIGURATION": "Configuration",
"GUI_LANGUAGE": "Langue",
"GUI_UNITS": "Unités",
"GUI_KILOMETERS": "Kilomètres",
"GUI_MILES": "Miles",
// mapping
"GUI_MAPPING_TOOLS": "Outils de cartographie",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Surligner les rues sans nom",
"GUI_SHOW_PREVIOUS_ROUTES": "Afficher itinéraires précédents",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Résultats de recherche",
"FOUND_X_RESULTS": "%i résultat(s)",
"TIMED_OUT": "La recherche n’a pas abouti",
"NO_RESULTS_FOUND": "Aucun résultat trouvé",
"NO_RESULTS_FOUND_SOURCE": "Aucun résultat pour le départ",
"NO_RESULTS_FOUND_TARGET": "Aucun résultat pour l'arrivée",
// routing
"ROUTE_DESCRIPTION": "Description de l’itinéraire",
"GET_LINK_TO_ROUTE": "Générer un lien",
"GENERATE_LINK_TO_ROUTE": "en attente du lien",
"LINK_TO_ROUTE_TIMEOUT": "indisponible",
"GPX_FILE": "Fichier GPX",
"DISTANCE": "Distance",
"DURATION": "Durée",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Votre itinéraire est en cours de calcul",
"NO_ROUTE_FOUND": "Pas d’itinéraire possible",
// printing
"OVERVIEW_MAP": "Carte",
"NO_ROUTE_SELECTED": "Pas d’itinéraire choisi",
// routing engines
"ENGINE_0": "voiture (le plus rapide)",
// directions
"N": "nord",
"E": "est",
"S": "sud",
"W": "ouest",
"NE": "nord-est",
"SE": "sud-est",
"SW": "sud-ouest",
"NW": "nord-ouest",
// driving directions
// %s: road name
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Instruction inconnue[ sur <b>%s</b>]",
"DIRECTION_1":"Continuez[ sur <b>%s</b>]",
"DIRECTION_2":"Tournez légèrement à droite[ sur <b>%s</b>]",
"DIRECTION_3":"Tournez à droite[ sur <b>%s</b>]",
"DIRECTION_4":"Tournez fortement à droite[ sur <b>%s</b>]",
"DIRECTION_5":"Faites demi-tour[ sur <b>%s</b>]",
"DIRECTION_6":"Tournez fortement à gauche[ sur <b>%s</b>]",
"DIRECTION_7":"Tournez à gauche[ sur <b>%s</b>]",
"DIRECTION_8":"Tournez légèrement à gauche[ sur <b>%s</b>]",
"DIRECTION_10":"Direction <b>%d</b>[ sur <b>%s</b>]",
"DIRECTION_11-1":"Au rond-point, prenez la première sortie[ sur <b>%s</b>]",
"DIRECTION_11-2":"Au rond-point, prenez la deuxième sortie[ sur <b>%s</b>]",
"DIRECTION_11-3":"Au rond-point, prenez la troisième sortie[ sur <b>%s</b>]",
"DIRECTION_11-4":"Au rond-point, prenez la quatrième sortie[ sur <b>%s</b>]",
"DIRECTION_11-5":"Au rond-point, prenez la cinquième sortie[ sur <b>%s</b>]",
"DIRECTION_11-6":"Au rond-point, prenez la sixième sortie[ sur <b>%s</b>]",
"DIRECTION_11-7":"Au rond-point, prenez la septième sortie[ sur <b>%s</b>]",
"DIRECTION_11-8":"Au rond-point, prenez la huitième sortie[ sur <b>%s</b>]",
"DIRECTION_11-9":"Au rond-point, prenez la neuvième sortie[ sur <b>%s</b>]",
"DIRECTION_11-x":"Au rond-point, prenez l’une des trop nombreuses sorties[ sur <b>%s</b>]",
"DIRECTION_15":"Vous êtes arrivé"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("fr");
