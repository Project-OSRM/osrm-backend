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

// OSRM MainGUI
// [handles all GUI events that interact with appearance of main window]


OSRM.GUI.extend( {
		
// init GUI
init: function() {
	// init main box
	var main_group = new OSRM.GUIBoxGroup();
	OSRM.G.main_handle = new OSRM.GUIBoxHandle("main", "left", "left:-5px;top:5px;", OSRM.GUI.beforeMainTransition, OSRM.GUI.afterMainTransition);
	main_group.add( OSRM.G.main_handle );
	main_group.select( OSRM.G.main_handle );
	
	// init additional boxes
	var option_group = new OSRM.GUIBoxGroup();
	var config_handle = new OSRM.GUIBoxHandle("config", "right", "right:-5px;bottom:70px;");
	var mapping_handle = new OSRM.GUIBoxHandle("mapping", "right", "right:-5px;bottom:25px;");
	option_group.add( config_handle );
	option_group.add( mapping_handle );
	option_group.select( null );
	
	// init starting source/target
	document.getElementById('gui-input-source').value = OSRM.DEFAULTS.ONLOAD_SOURCE;
	document.getElementById('gui-input-target').value = OSRM.DEFAULTS.ONLOAD_TARGET;
	
	//init units selector
	OSRM.GUI.initDistanceFormatsSelector();
},

// set language dependent labels
setLabels: function() {
	document.getElementById("open-josm").innerHTML = OSRM.loc("OPEN_JOSM");
	document.getElementById("open-osmbugs").innerHTML = OSRM.loc("OPEN_OSMBUGS");
	document.getElementById("gui-reset").innerHTML = OSRM.loc("GUI_RESET");
	document.getElementById("gui-reverse").innerHTML = OSRM.loc("GUI_REVERSE");
	document.getElementById("gui-option-highlight-nonames-label").lastChild.nodeValue = OSRM.loc("GUI_HIGHLIGHT_UNNAMED_ROADS");
	document.getElementById("gui-option-show-previous-routes-label").lastChild.nodeValue = OSRM.loc("GUI_SHOW_PREVIOUS_ROUTES");
	document.getElementById("gui-search-source").innerHTML = OSRM.loc("GUI_SEARCH");
	document.getElementById("gui-search-target").innerHTML = OSRM.loc("GUI_SEARCH");
	document.getElementById("gui-search-source-label").innerHTML = OSRM.loc("GUI_START")+":";
	document.getElementById("gui-search-target-label").innerHTML = OSRM.loc("GUI_END")+":";
	document.getElementById("gui-input-source").title = OSRM.loc("GUI_START_TOOLTIP");
	document.getElementById("gui-input-target").title = OSRM.loc("GUI_END_TOOLTIP");
	document.getElementById("legal-notice").innerHTML = OSRM.loc("GUI_LEGAL_NOTICE");
	document.getElementById("gui-mapping-label").innerHTML = OSRM.loc("GUI_MAPPING_TOOLS");
	document.getElementById("gui-config-label").innerHTML = OSRM.loc("GUI_CONFIGURATION");
	document.getElementById("gui-language-2-label").innerHTML = OSRM.loc("GUI_LANGUAGE")+":";
	document.getElementById("gui-units-label").innerHTML = OSRM.loc("GUI_UNITS")+":";
	document.getElementById('gui-data-timestamp-label').innerHTML = OSRM.loc("GUI_DATA_TIMESTAMP");
	document.getElementById('gui-data-timestamp').innerHTML = OSRM.G.data_timestamp;
	document.getElementById('gui-timestamp-label').innerHTML = OSRM.loc("GUI_VERSION");	
	document.getElementById('gui-timestamp').innerHTML = OSRM.DATE+"; v"+OSRM.VERSION;	
	document.getElementById('config-handle-icon').title = OSRM.loc("GUI_CONFIGURATION");
	document.getElementById('mapping-handle-icon').title = OSRM.loc("GUI_MAPPING_TOOLS");
	document.getElementById('main-handle-icon').title = OSRM.loc("GUI_MAIN_WINDOW");
	OSRM.G.map.zoomControl.setTooltips( OSRM.loc("GUI_ZOOM_IN"), OSRM.loc("GUI_ZOOM_OUT") );
	OSRM.G.map.locationsControl.setTooltips( OSRM.loc("GUI_ZOOM_ON_USER"), OSRM.loc("GUI_ZOOM_ON_ROUTE") );
	OSRM.GUI.setDistanceFormatsLanguage();
	OSRM.GUI.setRoutingEnginesLanguage();	
},

// clear output area
clearResults: function() {
	document.getElementById('information-box').innerHTML = "";
	document.getElementById('information-box-header').innerHTML = "";	
},

// reposition and hide zoom controls before main box animation
beforeMainTransition: function() {
	OSRM.G.map.zoomControl.hide();
},
// show zoom controls after main box animation
afterMainTransition: function() {
	OSRM.G.map.zoomControl.show();
},

// distance format routines
initDistanceFormatsSelector: function() {
	var options = OSRM.GUI.getDistanceFormats();
	OSRM.GUI.selectorInit( "gui-units-toggle", options, OSRM.DEFAULTS.DISTANCE_FORMAT, OSRM.GUI._onDistanceFormatChanged );	
},
setDistanceFormat: function(type) {
	if( OSRM.G.active_distance_format == type )
		return;
	OSRM.G.active_distance_format = type;
	
	// change scale control
	if(OSRM.G.map) {
		OSRM.G.map.scaleControl.removeFrom(OSRM.G.map);
		OSRM.G.map.scaleControl.options.metric = (type != 1);
		OSRM.G.map.scaleControl.options.imperial = (type == 1);
		OSRM.G.map.scaleControl.addTo(OSRM.G.map);
	}
	
	// change converter
	if( type == 1 )
		OSRM.Utils.toHumanDistance = OSRM.Utils.toHumanDistanceMiles;
	else
		OSRM.Utils.toHumanDistance = OSRM.Utils.toHumanDistanceMeters;
},
_onDistanceFormatChanged: function(type) {
	OSRM.GUI.setDistanceFormat(type);
	OSRM.Routing.getRoute({keepAlternative:true});
},
setDistanceFormatsLanguage: function() {
	var options = OSRM.GUI.getDistanceFormats();
	OSRM.GUI.selectorRenameOptions("gui-units-toggle", options );	
},
getDistanceFormats: function() {
	return [{display:OSRM.loc("GUI_KILOMETERS"),value:0},{display:OSRM.loc("GUI_MILES"),value:1}];
},

// data timestamp routines
queryDataTimestamp: function() {
	OSRM.G.data_timestamp = "n/a";	
	OSRM.JSONP.call( OSRM.G.active_routing_timestamp_url+"?jsonp=%jsonp", OSRM.GUI.setDataTimestamp, OSRM.JSONP.empty, OSRM.DEFAULTS.JSONP_TIMEOUT, 'data_timestamp');	
},
setDataTimestamp: function(response) {
	if(!response)
		return;

	OSRM.G.data_timestamp = response.timestamp.slice(0,25).replace(/<\/?[^>]+(>|$)/g ,"");	// discard tags
	document.getElementById('gui-data-timestamp').innerHTML = OSRM.G.data_timestamp;
}

});