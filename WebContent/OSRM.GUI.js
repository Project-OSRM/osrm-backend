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

// OSRM GUI functionality
// [responsible for all non-routing related GUI behaviour]


OSRM.GUI = {
		
// defaults
visible: null,
width: null,

// init GUI
init: function() {
	OSRM.GUI.visible = true;
	OSRM.GUI.width = document.getElementById("main-wrapper").clientWidth;
	
	// translate
	document.getElementById("open-josm").innerHTML = OSRM.loc("OPEN_JOSM");
	document.getElementById("open-osmbugs").innerHTML = OSRM.loc("OPEN_OSMBUGS");	
	document.getElementById("gui-reset").innerHTML = OSRM.loc("GUI_RESET");
	document.getElementById("gui-reverse").innerHTML = OSRM.loc("GUI_REVERSE");
	document.getElementById("gui-option-highlight-nonames-label").innerHTML = OSRM.loc("GUI_HIGHLIGHT_UNNAMED_ROADS");
	document.getElementById("options-toggle").innerHTML = OSRM.loc("GUI_OPTIONS");
	document.getElementById("gui-search-source").innerHTML = OSRM.loc("GUI_SEARCH");
	document.getElementById("gui-search-target").innerHTML = OSRM.loc("GUI_SEARCH");
	document.getElementById("gui-search-source-label").innerHTML = OSRM.loc("GUI_START")+":";
	document.getElementById("gui-search-target-label").innerHTML = OSRM.loc("GUI_END")+":";
	document.getElementById("input-source-name").title = OSRM.loc("GUI_START_TOOLTIP");
	document.getElementById("input-target-name").title = OSRM.loc("GUI_END_TOOLTIP");
	document.getElementById("legal-notice").innerHTML = OSRM.loc("GUI_LEGAL_NOTICE");
	
	document.getElementById('input-source-name').value = OSRM.DEFAULTS.ONLOAD_SOURCE;
	document.getElementById('input-target-name').value = OSRM.DEFAULTS.ONLOAD_TARGET;	
},
		
// show/hide main-gui
toggleMain: function() {
	// show main-gui
	if( OSRM.GUI.visible == false ) {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="hidden";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.left=(OSRM.GUI.width+10)+"px";;
		
		document.getElementById('blob-wrapper').style.visibility="hidden";
		document.getElementById('main-wrapper').style.left="5px";
	// hide main-gui
	} else {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="hidden";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.left="30px";
			
		document.getElementById('main-wrapper').style.left=-OSRM.GUI.width+"px";
	}

	// execute after animation
	if( OSRM.Browser.FF3==-1 && OSRM.Browser.IE6_9==-1 ) {
		document.getElementById('main-wrapper').addEventListener("transitionend", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("webkitTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("oTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("MSTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
	} else {
		OSRM.GUI.onMainTransitionEnd();		
	}
},

// do stuff after main-gui animation finished
onMainTransitionEnd: function() {
	// after hiding main-gui
	if( OSRM.GUI.visible == true ) {
		document.getElementById('blob-wrapper').style.visibility="visible";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";
		OSRM.GUI.visible = false;		
	// after showing main-gui
	} else {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";
 		OSRM.GUI.visible = true;		
	}
},

// show/hide small options bubble
toggleOptions: function() {
	if(document.getElementById('options-box').style.visibility=="visible") {
		document.getElementById('options-box').style.visibility="hidden";
	} else {
		document.getElementById('options-box').style.visibility="visible";
	}
}

};