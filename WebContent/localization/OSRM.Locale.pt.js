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
// [Portugese language support]


OSRM.Localization["pt"] = {
// own language
"CULTURE": "pt-PT",
"LANGUAGE": "Portugues",
// gui
"GUI_START": "Iniciar",
"GUI_END": "Terminar",
"GUI_RESET": "Restabelecer",
"GUI_ZOOM_ON_ROUTE": "Ampliar para Rota",
"GUI_ZOOM_ON_USER": "Ampliar para Utilizador",
"GUI_SEARCH": "Mostrar",
"GUI_REVERSE": "Inverso",
"GUI_START_TOOLTIP": "Indique início",
"GUI_END_TOOLTIP": "Indique destino",
"GUI_MAIN_WINDOW": "Janela principal",
"GUI_ZOOM_IN": "Aumentar",
"GUI_ZOOM_OUT": "Reduzir",
// config
"GUI_CONFIGURATION": "Configuração",
"GUI_LANGUAGE": "Idioma",
"GUI_UNITS": "Unidades",
"GUI_KILOMETERS": "Quilometros",
"GUI_MILES": "Milhas",
// mapping
"GUI_MAPPING_TOOLS": "Ferramentas de Mapear",
"GUI_HIGHLIGHT_UNNAMED_ROADS": "Destaque ruas sem nome",
"GUI_SHOW_PREVIOUS_ROUTES": "Mostrar rotas anteriores",
"OPEN_JOSM": "JOSM",
"OPEN_OSMBUGS": "OSM Bugs",
// geocoder
"SEARCH_RESULTS": "Pesquisar resultados",
"FOUND_X_RESULTS": "encontrados %i resultados",
"TIMED_OUT": "Esgotado o tempo limite",
"NO_RESULTS_FOUND": "Não foram encontrados resultados",
"NO_RESULTS_FOUND_SOURCE": "Não foram encontrados resultados para o início",
"NO_RESULTS_FOUND_TARGET": "Não foram encontrados resultados para o final",
// routing
"ROUTE_DESCRIPTION": "Descrição da rota",
"GET_LINK_TO_ROUTE": "gerar ligação",
"GENERATE_LINK_TO_ROUTE": "à espera de ligação",
"LINK_TO_ROUTE_TIMEOUT": "não disponível",
"GPX_FILE": "Ficheiro GPX",
"DISTANCE": "Distância",
"DURATION": "Duração",
"YOUR_ROUTE_IS_BEING_COMPUTED": "A sua rota está a ser calculada",
"NO_ROUTE_FOUND": "A rota não é possivel",
// printing
"OVERVIEW_MAP": "Mapa de visão global",
"NO_ROUTE_SELECTED": "Sem rota selecionada",
// routing engines
"ENGINE_0": "Carro (mais rápido)",
// directions
"N": "norte",
"E": "este",
"S": "sul",
"W": "oeste",
"NE": "nordeste",
"SE": "sudeste",
"SW": "sudoeste",
"NW": "noroeste",
// driving directions
// %s: road name
// %d: direction
// [*]: will only be printed when there actually is a road name
"DIRECTION_0":"Instrução desconhecida[ para <b>%s</b>]",
"DIRECTION_1":"Continuar[ para <b>%s</b>]",
"DIRECTION_2":"Virar ligeiramente à direita[ para <b>%s</b>]",
"DIRECTION_3":"Virar à direita[ para <b>%s</b>]",
"DIRECTION_4":"Virar à direita apertada[ para <b>%s</b>]",
"DIRECTION_5":"Inversão de marcha[ para <b>%s</b>]",
"DIRECTION_6":"Virar à esquerda apertada[ para <b>%s</b>]",
"DIRECTION_7":"Virar à esquerda[ para <b>%s</b>]",
"DIRECTION_8":"Virar ligeiramente à esquerda[ para <b>%s</b>]",
"DIRECTION_10":"Em direção a <b>%d</b>[ para <b>%s</b>]",
"DIRECTION_11-1":"Entrar na rotunda e sair na primeira saída[ para <b>%s</b>]",
"DIRECTION_11-2":"Entrar na rotunda e sair na segunda saída[ para <b>%s</b>]",
"DIRECTION_11-3":"Entrar na rotunda e sair na terceira saída[ para <b>%s</b>]",
"DIRECTION_11-4":"Entrar na rotunda e sair na quarta saída[ para <b>%s</b>]",
"DIRECTION_11-5":"Entrar na rotunda e sair na quinta saída[ para <b>%s</b>]",
"DIRECTION_11-6":"Entrar na rotunda e sair na sexta saída[ para <b>%s</b>]",
"DIRECTION_11-7":"Entrar na rotunda e sair na sétima saída[ para <b>%s</b>]",
"DIRECTION_11-8":"Entrar na rotunda e sair na oitava saída[ para <b>%s</b>]",
"DIRECTION_11-9":"Entrar na rotunda e sair na nona saída[ para <b>%s</b>]",
"DIRECTION_11-x":"Entrar na rotunda e sair numa das saídas[ para <b>%s</b>]",
"DIRECTION_15":"Você chegou ao seu destino",
// notifications
"NOTIFICATION_MAINTENANCE_HEADER":	"Manutenção Programada",
"NOTIFICATION_MAINTENANCE_BODY":	"O Site do OSRM é desligado devido a uma manutenção programada. " + 
									"Por favor, seja paciente enquanto as atualizações necessárias são realizadas. " +
									"O site estará de volta online em breve." +
									"<br/><br/>" + 
									"Entretanto poderá sair e mapear um bairro neighborhood perto de si..." + 
									"<br/><br/><br/>[OSRM]",
"NOTIFICATION_LOCALIZATION_HEADER":	"Você sabe? Pode alterar o idioma do ecran de exibição.",
"NOTIFICATION_LOCALIZATION_BODY":	"Pode usar o menu suspenso no canto superior esquerdo para selecionar o seu idioma preferido. " +
									"<br/><br/>" +
									"Não desespere se não consegue encontrar o idioma de sua escolha. " +
									"Se quiser, você pode ajudar a fornecer traduções adicionais! " +
									"Visite <a href='https://github.com/DennisSchiefer/Project-OSRM-Web'>aqui</a> para mais informações.",
"NOTIFICATION_CLICKING_HEADER":		"Você sabe? Pode clicar no mapa para definir marcadores de rota.",
"NOTIFICATION_CLICKING_BODY":		"Clique no mapa com o botão esquerdo do rato para definir um marcador fonte (verde) ou um marcador de destino (vermelho), " +
									"se o marcador de origem já existe. " +
									"O endereço do local selecionado será exibido nas caixas à esquerda. " + 
									"<br/><br/>" +
									"Pode excluir um marcador, com um clique sobre ele novamente com o botão esquerdo do rato.",
"NOTIFICATION_DRAGGING_HEADER":		"Você sabe? Pode arrastar cada marcador de rota no mapa.",
"NOTIFICATION_DRAGGING_BODY":		"Pode arrastar um marcador, com um clique sobre ele com o botão esquerdo do rato e mantendo o botão pressionado. " +
									"Então pode mover o marcador ao redor do mapa e a rota será atualizada instantaneamente. " +
									"<br/><br/>" +
									"Até pode criar marcadores intermédios, arrastando-os para fora da rota principal! "
};

// set GUI language on load
if( OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING == true )
	OSRM.Localization.setLanguage("pt");
