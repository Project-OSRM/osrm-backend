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
// [management of routing requests and processing of responses]

// some variables
OSRM.GLOBALS.route = null;
OSRM.GLOBALS.markers = null;

OSRM.GLOBALS.dragging = null;
OSRM.GLOBALS.dragid = null;
OSRM.GLOBALS.pending = false;


OSRM.Routing = {
		
// init routing data structures
init: function() {
	OSRM.G.markers = new OSRM.Markers();	
	OSRM.G.route = new OSRM.Route();
},


// -- JSONP processing -- 

// process JSONP response of routing server
timeoutRoute: function() {
	OSRM.RoutingGeometry.showNA();
	OSRM.RoutingNoNames.showNA();
	OSRM.RoutingDescription.showNA( OSRM.loc("TIMED_OUT") );
	OSRM.Routing._snapRoute();	
},
timeoutRoute_Dragging: function() {
	OSRM.RoutingGeometry.showNA();
	OSRM.RoutingDescription.showNA( OSRM.loc("TIMED_OUT") );
},
timeoutRoute_Reversed: function() {
	OSRM.G.markers.reverseMarkers();
	timeoutRoute();
},
showRoute: function(response) {
	if(!response)
		return;
	
	OSRM.G.response = response;	// needed for printing & history routes!
	if(response.status == 207) {
		OSRM.RoutingGeometry.showNA();
		OSRM.RoutingNoNames.showNA();
		OSRM.RoutingDescription.showNA( OSRM.loc("NO_ROUTE_FOUND") );
		OSRM.Routing._snapRoute();		
	} else {
		OSRM.RoutingGeometry.show(response);
		OSRM.RoutingNoNames.show(response);
		OSRM.RoutingDescription.show(response);
		OSRM.Routing._snapRoute();
	}
	OSRM.Routing._updateHints(response);
},
showRoute_Dragging: function(response) {
 	if(!response)
 		return;
 	if( !OSRM.G.dragging )		// prevent simple routing when not dragging (required as there can be drag events after a dragstop event!)
 		return;

	OSRM.G.response = response;	// needed for history routes!
	if( response.status == 207) {
		OSRM.RoutingGeometry.showNA();
		OSRM.RoutingDescription.showNA( OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED") );
	} else {
		OSRM.RoutingGeometry.show(response);
		OSRM.RoutingDescription.showSimple(response);
	}
	OSRM.Routing._updateHints(response);

	if(OSRM.G.pending)
		setTimeout(OSRM.Routing.draggingTimeout,1);		
},
showRoute_Redraw: function(response) {
	if(!response)
		return;
	
	//OSRM.G.response = response;	// not needed, even harmful as important information is not stored!
	if(response.status != 207) {
		OSRM.RoutingGeometry.show(response);
		OSRM.RoutingNoNames.show(response);
	}
	OSRM.Routing._updateHints(response);
},


//-- main function --

//generate server calls to query routes
getRoute: function() {
	
	// if source or target are not set -> hide route
	if( OSRM.G.markers.route.length < 2 ) {
		OSRM.G.route.hideRoute();
		return;
	}
	OSRM.JSONP.clear('dragging');
	OSRM.JSONP.clear('redraw');
	OSRM.JSONP.clear('route');
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&instructions=true', OSRM.Routing.showRoute, OSRM.Routing.timeoutRoute, OSRM.DEFAULTS.JSONP_TIMEOUT, 'route');
},
getRoute_Reversed: function() {
	if( OSRM.G.markers.route.length < 2 )
		return;
	
	OSRM.JSONP.clear('dragging');
	OSRM.JSONP.clear('redraw');
	OSRM.JSONP.clear('route');
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&instructions=true', OSRM.Routing.showRoute, OSRM.Routing.timeoutRoute_Reversed, OSRM.DEFAULTS.JSONP_TIMEOUT, 'route');	
},
getRoute_Redraw: function() {
	if( OSRM.G.markers.route.length < 2 )
		return;
	
	OSRM.JSONP.clear('dragging');
	OSRM.JSONP.clear('redraw');
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&instructions=true', OSRM.Routing.showRoute_Redraw, OSRM.Routing.timeoutRoute, OSRM.DEFAULTS.JSONP_TIMEOUT, 'redraw');
},
getRoute_Dragging: function() {
	OSRM.G.pending = !OSRM.JSONP.call(OSRM.Routing._buildCall()+'&instructions=false', OSRM.Routing.showRoute_Dragging, OSRM.Routing.timeoutRoute_Dragging, OSRM.DEFAULTS.JSONP_TIMEOUT, 'dragging');;
},
draggingTimeout: function() {
	OSRM.G.markers.route[OSRM.G.dragid].hint = null;
	OSRM.Routing.getRoute_Dragging();
},

_buildCall: function() {
	var source = OSRM.DEFAULTS.HOST_ROUTING_URL;
	source += '?z=' + OSRM.G.map.getZoom() + '&output=json&jsonp=%jsonp&geomformat=cmp';	
	if(OSRM.G.markers.checksum)
		source += '&checksum=' + OSRM.G.markers.checksum;
	var markers = OSRM.G.markers.route;
	for(var i=0,size=markers.length; i<size; i++) {
		source += '&loc='  + markers[i].getLat().toFixed(6) + ',' + markers[i].getLng().toFixed(6);
		if( markers[i].hint)
			source += '&hint=' + markers[i].hint;
	}
	return source;
},


//-- helper functions --

// update hints of all markers
_updateHints: function(response) {
	var hint_locations = response.hint_data.locations;
	OSRM.G.markers.checksum = response.hint_data.checksum;
	for(var i=0; i<hint_locations.length; i++)
		OSRM.G.markers.route[i].hint = hint_locations[i];
},

// snap all markers to the received route
_snapRoute: function() {
	var positions = OSRM.G.route.getPositions();
	var markers = OSRM.G.markers.route;
	var via_points = OSRM.G.response.via_points;
	
 	markers[0].setPosition( positions[0] );
 	markers[markers.length-1].setPosition( positions[positions.length-1] );
 	for(var i=0; i<via_points.length; i++)
 		markers[i+1].setPosition( new L.LatLng(via_points[i][0], via_points[i][1]) );

 	OSRM.Geocoder.updateAddress(OSRM.C.SOURCE_LABEL);
 	OSRM.Geocoder.updateAddress(OSRM.C.TARGET_LABEL);
}

};
