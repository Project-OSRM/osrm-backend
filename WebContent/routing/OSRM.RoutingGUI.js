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

// OSRM routing
// [handles GUI events]


OSRM.RoutingGUI = {

// click: button "reset"
resetRouting: function() {
	document.getElementById('input-source-name').value = "";
	document.getElementById('input-target-name').value = "";
	
	OSRM.G.route.hideAll();
	OSRM.G.markers.removeAll();
	OSRM.G.markers.highlight.hide();
	
	document.getElementById('information-box').innerHTML = "";
	document.getElementById('information-box-headline').innerHTML = "";
	
	OSRM.JSONP.reset();	
},

// click: button "reverse"
reverseRouting: function() {
	// invert input boxes
	var tmp = document.getElementById("input-source-name").value;
	document.getElementById("input-source-name").value = document.getElementById("input-target-name").value;
	document.getElementById("input-target-name").value = tmp;
	
	// invert route
	OSRM.G.markers.route.reverse();
	if(OSRM.G.markers.route.length == 1) {
		if(OSRM.G.markers.route[0].label == OSRM.C.TARGET_LABEL) {
			OSRM.G.markers.route[0].label = OSRM.C.SOURCE_LABEL;
			OSRM.G.markers.route[0].marker.setIcon( new L.Icon('images/marker-source.png') );
		} else if(OSRM.G.markers.route[0].label == OSRM.C.SOURCE_LABEL) {
			OSRM.G.markers.route[0].label = OSRM.C.TARGET_LABEL;
			OSRM.G.markers.route[0].marker.setIcon( new L.Icon('images/marker-target.png') );
		}
	} else if(OSRM.G.markers.route.length > 1){
		OSRM.G.markers.route[0].label = OSRM.C.SOURCE_LABEL;
		OSRM.G.markers.route[0].marker.setIcon( new L.Icon('images/marker-source.png') );
		
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].label = OSRM.C.TARGET_LABEL;
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].marker.setIcon( new L.Icon('images/marker-target.png') );		
	}
	
	// recompute route
	if( OSRM.G.route.isShown() ) {
		OSRM.Routing.getRoute();
		OSRM.G.markers.highlight.hide();
	} else {
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-headline').innerHTML = "";		
	}
},

// click: button "show"
showMarker: function(marker_id) {
	if( OSRM.JSONP.fences["geocoder_source"] || OSRM.JSONP.fences["geocoder_target"] )
		return;
	
	if( marker_id == OSRM.C.SOURCE_LABEL && OSRM.G.markers.hasSource() )
		OSRM.G.markers.route[0].centerView();
	else if( marker_id == OSRM.C.TARGET_LABEL && OSRM.G.markers.hasTarget() )
		OSRM.G.markers.route[OSRM.G.markers.route.length-1].centerView();
},


// changed: any inputbox (is called when return is pressed [after] or focus is lost [before])
inputChanged: function(marker_id) {
	if( marker_id == OSRM.C.SOURCE_LABEL)	
		OSRM.Geocoder.call(OSRM.C.SOURCE_LABEL, document.getElementById('input-source-name').value);
	else if( marker_id == OSRM.C.TARGET_LABEL)
		OSRM.Geocoder.call(OSRM.C.TARGET_LABEL, document.getElementById('input-target-name').value);
},

// click: button "open JOSM"
openJOSM: function() {
	var x = OSRM.G.map.getCenterUI();
	var ydelta = 0.01;
	var xdelta = ydelta * 2;
	var p = [ 'left='  + (x.lng - xdelta), 'bottom=' + (x.lat - ydelta), 'right=' + (x.lng + xdelta), 'top='    + (x.lat + ydelta)];
	var url = 'http://localhost:8111/load_and_zoom?' + p.join('&');
	
	var frame = L.DomUtil.create('iframe', null, document.body);
	frame.style.width = frame.style.height = "0px";
	frame.src = url;
	frame.onload = function(e) { document.body.removeChild(frame); };
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
}

};