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
		
// show/hide main-gui
toggleMain: function() {
	// show main-gui
	if( document.getElementById('main-wrapper').style.left == "-410px" ) {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="hidden";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.left="420px";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.top="5px";
		
		document.getElementById('blob-wrapper').style.visibility="hidden";			
		document.getElementById('main-wrapper').style.left="5px";
		if( OSRM.Browser.FF3!=-1 || OSRM.Browser.IE6_9!=-1 ) {
			getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";
		}
	// hide main-gui
	} else {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="hidden";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.left="30px";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.top="5px";
			
		document.getElementById('main-wrapper').style.left="-410px";
 		if( OSRM.Browser.FF3!=-1 || OSRM.Browser.IE6_9!=-1 ) {
 			document.getElementById('blob-wrapper').style.visibility="visible";
			getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";		
 		}
	}

	// execute after animation
	if( OSRM.Browser.FF3==-1 && OSRM.Browser.IE6_9==-1 ) {
		document.getElementById('main-wrapper').addEventListener("transitionend", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("webkitTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("oTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("MSTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
	}
},

// do stuff after main-gui animation finished
onMainTransitionEnd: function() {
	// after hiding main-gui
	if( document.getElementById('main-wrapper').style.left == "-410px" ) {
		document.getElementById('blob-wrapper').style.visibility="visible";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";
	// after showing main-gui
	} else {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";
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