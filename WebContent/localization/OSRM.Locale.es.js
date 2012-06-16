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
// [Spanish language support]


OSRM.Localization["es"] = {
// own language
"LANGUAGE": "Español",
// gui
"GUI_START": "Inicio",
"GUI_END": "Destino",
"GUI_RESET": "Borrar",
"GUI_ZOOM": "Zoom en la Ruta",
"GUI_SEARCH": "Mostrar",
"GUI_REVERSE": "Invertir",
"GUI_START_TOOLTIP": "Escriba la dirección de origen",
"GUI_END_TOOLTIP": "Escriba la dirección de destino",
"GUI_LEGAL_NOTICE": "GUI2 v"+OSRM.VERSION+" "+OSRM.DATE+" - OSRM alojado en <a href='http://algo2.iti.kit.edu/'>KIT</a> - Geocoder gracias a <a href='http://www.osm.org/'>OSM</a>",
// config
"GUI_CONFIGURATION": "Configuración",
"GUI_LANGUAGE": "Idioma",
"GUI_UNITS": "Unidades",
"GUI_KILOMETERS": "Kilometros",
"GUI_MILES": "Millas",
"GUI_DATA_TIMESTAMP": "datos",
// mapping
"GUI_MAPPING_TOOLS": "Herramientas del mapa",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Resaltar calles sin nombre",
"GUI_SHOW_PREVIOUS_ROUTES": "Mostrar rutas anteriores",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Resultados de la búsqueda",
"FOUND_X_RESULTS": "%i resultado(s)",
"TIMED_OUT": "Se superó el tiempo máximo de espera",
"NO_RESULTS_FOUND": "No se han encontrado resultados",
"NO_RESULTS_FOUND_SOURCE": "Ningún resultado para el inicio",
"NO_RESULTS_FOUND_TARGET": "Ningún resultado para el destino",
// routing
"ROUTE_DESCRIPTION": "Descripción de la ruta",
"GET_LINK_TO_ROUTE": "Generar enlace",
"GENERATE_LINK_TO_ROUTE": "esperando el enlace",
"LINK_TO_ROUTE_TIMEOUT": "no disponible",
"GPX_FILE": "Archivo GPX",
"DISTANCE": "Distancia",
"DURATION": "Duración",
"YOUR_ROUTE_IS_BEING_COMPUTED": "Estamos calculando su ruta",
"NO_ROUTE_FOUND": "No hay ninguna ruta posible",
// printing
"OVERVIEW_MAP": "Mapa de referencia",
"NO_ROUTE_SELECTED": "Ninguna ruta seleccionada",
// directions
"N": "norte",
"E": "este",
"S": "sur",
"W": "oeste",
"NE": "noreste",
"SE": "sureste",
"SW": "suroeste",
"NW": "noroeste",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Instrucción desconocida[ en <b>%s</b>]",
"DIRECTION_1":"Continúe[ por <b>%s</b>]",
"DIRECTION_2":"Gire ligeramente a la derecha[ hacia <b>%s</b>]",
"DIRECTION_3":"Gire a la derecha [ hacia <b>%s</b>]",
"DIRECTION_4":"Gire pronunciadamente a la derecha[ hacia <b>%s</b>]",
"DIRECTION_5":"Realice un cambio de sentido[ en <b>%s</b>]",
"DIRECTION_6":"Gire pronunciadamente a la izquierda[ hacia <b>%s</b>]",
"DIRECTION_7":"Gire a la izquierda [ hacia <b>%s</b>]",
"DIRECTION_8":"Gire ligeramente a la izquierda[ hacia <b>%s</b>]",
"DIRECTION_10":"Diríjase hacia el <b>%d</b>[ por <b>%s</b>]",
"DIRECTION_11-1":"En la rotonda, tome la primera salida[ en dirección <b>%s</b>]",
"DIRECTION_11-2":"En la rotonda, tome la segunda salida[ en dirección <b>%s</b>]",
"DIRECTION_11-3":"En la rotonda, tome la tercera salida[ en dirección <b>%s</b>]",
"DIRECTION_11-4":"En la rotonda, tome la cuarta salida[ en dirección <b>%s</b>]",
"DIRECTION_11-5":"En la rotonda, tome la quinta salida[ en dirección <b>%s</b>]",
"DIRECTION_11-6":"En la rotonda, tome la sexta salida[ en dirección <b>%s</b>]",
"DIRECTION_11-7":"En la rotonda, tome la séptima salida[ en dirección <b>%s</b>]",
"DIRECTION_11-8":"En la rotonda, tome la octava salida[ en dirección <b>%s</b>]",
"DIRECTION_11-9":"En la rotonda, tome la novena salida[ en dirección <b>%s</b>]",
"DIRECTION_11-x":"En la rotonda, tome una de sus muchas salidas [ en dirección <b>%s</b>]",
"DIRECTION_15":"Ha llegado a su destino"
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("es");
