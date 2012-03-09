// GUI functionality


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
		if( OSRM.Browser.FF3!=-1 || OSRM.Browser.IE6_8!=-1 ) {
			getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";
		}
	// hide main-gui
	} else {
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="hidden";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.left="30px";
		getElementsByClassName(document,'leaflet-control-zoom')[0].style.top="5px";
			
		document.getElementById('main-wrapper').style.left="-410px";
 		if( OSRM.Browser.FF3!=-1 || OSRM.Browser.IE6_8!=-1 ) {
 			document.getElementById('blob-wrapper').style.visibility="visible";
			getElementsByClassName(document,'leaflet-control-zoom')[0].style.visibility="visible";		
 		}
	}

	// execute after animation
	if( OSRM.Browser.FF3==-1 && OSRM.Browser.IE6_8==-1 ) {
		document.getElementById('main-wrapper').addEventListener("transitionend", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("webkitTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
		document.getElementById('main-wrapper').addEventListener("oTransitionEnd", OSRM.GUI.onMainTransitionEnd, false);
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
},

};