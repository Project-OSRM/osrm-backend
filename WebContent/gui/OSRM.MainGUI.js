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
	
	// init units selector
	OSRM.GUI.selectorInit( "gui-units-toggle", [{display:"Kilometers",value:0},{display:"Miles",value:1}], 0, OSRM.GUI.onUnitsChanged );
	
	// query last update of data
	OSRM.G.data_timestamp = "n/a";
	OSRM.JSONP.call(OSRM.DEFAULTS.HOST_TIMESTAMP_URL+"?jsonp=%jsonp", OSRM.GUI.setDataTimestamp, OSRM.JSONP.empty, OSRM.DEFAULTS.JSONP_TIMEOUT, 'data_timestamp');
},

// set language dependent labels
setLabels: function() {
	document.getElementById("open-josm").innerHTML = OSRM.loc("OPEN_JOSM");
	document.getElementById("open-osmbugs").innerHTML = OSRM.loc("OPEN_OSMBUGS");	
	document.getElementById("gui-reset").innerHTML = OSRM.loc("GUI_RESET");
	document.getElementById("gui-zoom").innerHTML = OSRM.loc("GUI_ZOOM");
	document.getElementById("gui-reverse").innerHTML = OSRM.loc("GUI_REVERSE");
	document.getElementById("gui-option-highlight-nonames-label").innerHTML = OSRM.loc("GUI_HIGHLIGHT_UNNAMED_ROADS");
	document.getElementById("gui-option-show-previous-routes-label").innerHTML = OSRM.loc("GUI_SHOW_PREVIOUS_ROUTES");
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
	document.getElementById('gui-data-timestamp').innerHTML = OSRM.loc("GUI_DATA_TIMESTAMP")+": " + OSRM.G.data_timestamp;
	
	document.getElementById("gui-units-toggle").getElementsByTagName("option")[0].innerHTML = OSRM.loc("GUI_KILOMETERS");
	document.getElementById("gui-units-toggle").getElementsByTagName("option")[1].innerHTML = OSRM.loc("GUI_MILES");
	OSRM.GUI.selectorOnChange( document.getElementById("gui-units-toggle") );
	OSRM.GUI.updateNotifications();
},

// clear output area
clearResults: function() {
	document.getElementById('information-box').innerHTML = "";
	document.getElementById('information-box-header').innerHTML = "";	
},

// reposition and hide zoom controls before main box animation
beforeMainTransition: function() {
	var zoom_controls = OSRM.Browser.getElementsByClassName(document,'leaflet-control-zoom');
	if( zoom_controls.length > 0)
		zoom_controls[0].style.visibility="hidden";
},

// show zoom controls after main box animation
afterMainTransition: function() {
	var zoom_controls = OSRM.Browser.getElementsByClassName(document,'leaflet-control-zoom');
	if( zoom_controls.length > 0) {
		zoom_controls[0].style.left = ( OSRM.G.main_handle.boxVisible() == true ? (OSRM.G.main_handle.boxWidth()+10) : "30") + "px";
		zoom_controls[0].style.visibility="visible";
	}
},

// toggle distance units
onUnitsChanged: function(value) {
	OSRM.Utils.setToHumanDistanceFunction(value);
	OSRM.Routing.getRoute({keepAlternative:true});
},

// set timestamp of data
setDataTimestamp: function(response) {
	if(!response)
		return;
	
	OSRM.G.data_timestamp = response.timestamp.slice(0,25).replace(/<\/?[^>]+(>|$)/g ,"");	// discard tags
	document.getElementById('gui-data-timestamp').innerHTML = OSRM.loc("GUI_DATA_TIMESTAMP")+": " + OSRM.G.data_timestamp;
}

});