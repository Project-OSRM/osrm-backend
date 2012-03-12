// debug code for OSRM
// (works faster than console.log in time-critical events)

OSRM.debug = {};


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

	OSRM.debug.content= document.createElement('div');
	OSRM.debug.content.id = "OSRM.debug-content";
	OSRM.debug.content.style.cssText = "position:absolute;bottom:0px;top:0px;width:380px;font-size:11px;overflow:auto;margin:5px;";	
	
	// add elements
	document.body.appendChild(wrapper);
	wrapper.appendChild(box);
	box.appendChild(OSRM.debug.content);
};
if(document.addEventListener)	// FF, CH
	document.addEventListener("DOMContentLoaded", OSRM.debug.init, false);
else	// IE
	OSRM.debug.init();


// working functions
OSRM.debug.log = function(text) {
	OSRM.debug.content.innerHTML += text + "<hr style='border:none; margin:2px; height:1px; color:#F0F0F0; background:#F0F0F0;'/>";
};
OSRM.debug.clear = function() {
	OSRM.debug.content.innerHTML = "";
};	
