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

// OSRM RoutingGUI
// [handles all GUI events that interact with routing in main window]


OSRM.GUI.extend( {
		
// init
init: function() {
	// init variables
	OSRM.GUI.setDistanceFormat(OSRM.DEFAULTS.DISTANCE_FORMAT);
	
	// init events
	document.getElementById("gui-input-source").onchange = function() {OSRM.GUI.inputChanged(OSRM.C.SOURCE_LABEL);};
	document.getElementById("gui-delete-source").onclick = function() {OSRM.GUI.deleteMarker(OSRM.C.SOURCE_LABEL);};
	document.getElementById("gui-search-source").onclick = function() {OSRM.GUI.showMarker(OSRM.C.SOURCE_LABEL);};	
	
	document.getElementById("gui-input-target").onchange = function() {OSRM.GUI.inputChanged(OSRM.C.TARGET_LABEL);};
	document.getElementById("gui-delete-target").onclick = function() {OSRM.GUI.deleteMarker(OSRM.C.TARGET_LABEL);};
	document.getElementById("gui-search-target").onclick = function() {OSRM.GUI.showMarker(OSRM.C.TARGET_LABEL);};
	
	document.getElementById("gui-reset").onclick = OSRM.GUI.resetRouting;
	document.getElementById("gui-reverse").onclick = OSRM.GUI.reverseRouting;
	document.getElementById("open-josm").onclick = OSRM.GUI.openJOSM;
	document.getElementById("open-osmbugs").onclick = OSRM.GUI.openOSMBugs;
	document.getElementById("option-highlight-nonames").onclick = OSRM.GUI.hightlightNonames;
	document.getElementById("option-show-previous-routes").onclick = OSRM.GUI.showPreviousRoutes;
},

// toggle GUI features that need a route to work
activateRouteFeatures: function() {
	OSRM.Printing.activate();
	OSRM.G.map.locationsControl.activateRoute();
	OSRM.G.active_shortlink = null;							// delete shortlink when new route is shown (RoutingDescription calls this method!)
},
deactivateRouteFeatures: function() {
	OSRM.Printing.deactivate();
	OSRM.G.map.locationsControl.deactivateRoute();
	OSRM.G.active_shortlink = null;							// delete shortlink when the route is hidden
},

// click: button "reset"
resetRouting: function() {
	document.getElementById('gui-input-source').value = "";
	document.getElementById('gui-input-target').value = "";
	
	OSRM.G.route.reset();
	OSRM.G.markers.reset();
	
	document.getElementById('information-box').innerHTML = "";
	document.getElementById('information-box-header').innerHTML = "";
	
	OSRM.JSONP.reset();	
},

// click: button "reverse"
reverseRouting: function() {
	// invert input boxes
	var tmp = document.getElementById("gui-input-source").value;
	document.getElementById("gui-input-source").value = document.getElementById("gui-input-target").value;
	document.getElementById("gui-input-target").value = tmp;
	
	// recompute route if needed
	if( OSRM.G.route.isShown() ) {
		OSRM.G.markers.route.reverse();
		OSRM.Routing.getRoute_Reversed();				// temporary route reversal for query, actual reversal done after receiving response
		OSRM.G.markers.route.reverse();
		OSRM.G.markers.highlight.hide();
		OSRM.RoutingDescription.showSimple( OSRM.G.response );
	
	// simply reverse markers		
	} else {
		OSRM.G.markers.reverseMarkers();		
	}
},

// click: button "show"
showMarker: function(marker_id) {
	if( OSRM.JSONP.fences["geocoder_source"] || OSRM.JSONP.fences["geocoder_target"] )	// needed when focus was on input box and user clicked on button
		return;
	
	if( marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource() )
		OSRM.G.markers.route[0].centerView();
	else if( marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() )
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].centerView();
},

// changed: any inputbox (is called when enter is pressed [after] or focus is lost [before])
inputChanged: function(marker_id) {
	if( marker_id == OSRM.C.SOURCE_LABEL)	
		OSRM.Geocoder.call(OSRM.C.SOURCE_LABEL, document.getElementById('gui-input-source').value);
	else if( marker_id == OSRM.C.TARGET_LABEL)
		OSRM.Geocoder.call(OSRM.C.TARGET_LABEL, document.getElementById('gui-input-target').value);
},

// click: button "open JOSM"
openJOSM: function() {
	var center = OSRM.G.map.getCenterUI();
	var bounds = OSRM.G.map.getBoundsUI();
	
	var xdelta = Math.min(0.02, Math.abs(bounds.getSouthWest().lng - center.lng) );
	var ydelta = Math.min(0.01, Math.abs(bounds.getSouthWest().lat - center.lat) );
	
	var p = [ 'left='  + (center.lng - xdelta).toFixed(6), 'bottom=' + (center.lat - ydelta).toFixed(6), 'right=' + (center.lng + xdelta).toFixed(6), 'top=' + (center.lat + ydelta).toFixed(6)];
	var url = 'http://127.0.0.1:8111/load_and_zoom?' + p.join('&');
 
	var frame = document.getElementById('josm-frame');
	if(!frame) {
		frame = L.DomUtil.create('iframe', null, document.body);
		frame.style.display = "none";
		frame.id = 'josm-frame';
	}
	frame.src = url;
},

//click: button "open OSM Bugs"
openOSMBugs: function() {
	var position = OSRM.G.map.getCenterUI();
	window.open( "http://osmbugs.org/?lat="+position.lat.toFixed(6)+"&lon="+position.lng.toFixed(6)+"&zoom="+OSRM.G.map.getZoom() );
},

//click: button "delete marker"
deleteMarker: function(marker_id) {
	var id = null;
	if(marker_id == 'source' && OSRM.G.markers.hasSource() )
		id = 0;
	else if(marker_id == 'target' && OSRM.G.markers.hasTarget() )
		id = OSRM.G.markers.route.length-1;
	if( id == null)
		return;
	
	OSRM.G.markers.removeMarker( id );
	OSRM.Routing.getRoute();
	OSRM.G.markers.highlight.hide();	
},

//click: checkbox "show previous routes"
showPreviousRoutes: function(value) {
	if( document.getElementById('option-show-previous-routes').checked == false)
		OSRM.G.route.deactivateHistoryRoutes();
	else
		OSRM.G.route.activateHistoryRoutes();
},

//click: button "zoom on route"
zoomOnRoute: function() {
	if( OSRM.G.route.isShown() == false )
		return;
	
	var bounds = new L.LatLngBounds( OSRM.G.route._current_route.getPositions() );
	OSRM.G.map.fitBoundsUI(bounds);	
},

//click: button "zoom on user"
zoomOnUser: function() {
	if (navigator.geolocation) 
		navigator.geolocation.getCurrentPosition(OSRM.Map.geolocationResponse);	
},

//click: toggle highlighting unnamed streets
hightlightNonames: function() {
	OSRM.Routing.getRoute_Redraw({keepAlternative:true});
}

});
