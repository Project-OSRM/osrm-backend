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

// debug code for OSRM
// [works better than console.log in older browsers and for logging event handling]

OSRM.debug = {};


// access functions
OSRM.debug.log = function(text) {
	OSRM.debug.content.innerHTML += text + "<hr style='border:none; margin:2px; height:1px; color:#F0F0F0; background:#F0F0F0;'/>";
	OSRM.debug.content.scrollTop = OSRM.debug.content.scrollHeight; 
};
OSRM.debug.clear = function() {
	OSRM.debug.content.innerHTML = "";
};	


// add elements to DOM
OSRM.debug.init = function() {
	//create DOM objects for debug output
	var wrapper = document.createElement('div');
	wrapper.id = "OSRM.debug-wrapper";
	wrapper.className = "gui-wrapper";
	wrapper.style.cssText = "width:410px;height:95%;top:5px;right:50px;";

	var box = document.createElement('div');
	box.id = "OSRM.debug-box";
	box.className = "gui-box";
	box.style.cssText = "width:390px;top:0px;bottom:0px;";
	
	var clear = document.createElement('a');
	clear.id = "OSRM.debug-clear";
	clear.className = "button";
	clear.innerHTML = "clear";
	clear.onclick = OSRM.debug.clear;

	OSRM.debug.content= document.createElement('div');
	OSRM.debug.content.id = "OSRM.debug-content";
	OSRM.debug.content.style.cssText = "position:absolute;bottom:0px;top:20px;width:380px;font-size:11px;overflow:auto;margin:5px;";
		
	// add elements
	document.body.appendChild(wrapper);
	wrapper.appendChild(box);
	box.appendChild(clear);
	box.appendChild(OSRM.debug.content);
};


// onload event
OSRM.Browser.onLoadHandler( OSRM.debug.init );