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
OSRM.GLOBALS.pendingTimer = null;


OSRM.Routing = {
		
// init routing data structures
init: function() {
	OSRM.G.route = new OSRM.Route();
	OSRM.G.markers = new OSRM.Markers();
},


// -- JSONP processing -- 

// process JSONP response of routing server
timeoutRouteSimple: function() {
	OSRM.RoutingGeometry.showNA();
	OSRM.RoutingDescription.showNA( OSRM.loc("TIMED_OUT") );
},
timeoutRoute: function() {
	OSRM.RoutingGeometry.showNA();
	OSRM.RoutingNoNames.showNA();
	OSRM.RoutingDescription.showNA( OSRM.loc("TIMED_OUT") );
},
showRouteSimple: function(response) {
 	if(!response)
 		return;
 	if( !OSRM.G.dragging )		// prevent simple routing when no longer dragging
 		return;
 	
	if( response.status == 207) {
		OSRM.RoutingGeometry.showNA();
		OSRM.RoutingDescription.showNA( OSRM.loc("YOUR_ROUTE_IS_BEING_COMPUTED") );
	} else {
		OSRM.RoutingGeometry.show(response);
		OSRM.RoutingDescription.showSimple(response);
	}
	OSRM.Routing._updateHints(response);

	if(OSRM.G.pending)
		OSRM.G.pendingTimer = setTimeout(OSRM.Routing.draggingTimeout,1);		
},
showRoute: function(response) {
	if(!response)
		return;
	
	OSRM.G.via_points = response.via_points.slice(0);
	if(response.status == 207) {
		OSRM.RoutingGeometry.showNA();
		OSRM.RoutingNoNames.showNA();
		OSRM.RoutingDescription.showNA( OSRM.loc("NO_ROUTE_FOUND") );
	} else {
		OSRM.RoutingGeometry.show(response);
		OSRM.RoutingNoNames.show(response);
		OSRM.RoutingDescription.show(response);
		OSRM.Routing._snapRoute();
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
	
	OSRM.JSONP.call(OSRM.Routing._buildCall()+'&instructions=true', OSRM.Routing.showRoute, OSRM.Routing.timeoutRoute, OSRM.DEFAULTS.JSONP_TIMEOUT, 'route');
},
getDragRoute: function() {
	OSRM.G.pending = !OSRM.JSONP.call(OSRM.Routing._buildCall()+'&instructions=false', OSRM.Routing.showRouteSimple, OSRM.Routing.timeoutRouteSimple, OSRM.DEFAULTS.JSONP_TIMEOUT, 'dragging');;
},
draggingTimeout: function() {
	OSRM.G.markers.route[OSRM.G.dragid].hint = null;
	OSRM.Routing.getDragRoute();
},

_buildCall: function() {
	var source = OSRM.DEFAULTS.HOST_ROUTING_URL;
	source += '?z=' + OSRM.G.map.getZoom() + '&output=json&geomformat=cmp';	
	if(OSRM.G.markers.checksum)
		source += '&checksum=' + OSRM.G.markers.checksum;
	for(var i=0,size=OSRM.G.markers.route.length; i<size; i++) {
		source += '&loc='  + OSRM.G.markers.route[i].getLat() + ',' + OSRM.G.markers.route[i].getLng();
		if( OSRM.G.markers.route[i].hint)
			source += '&hint=' + OSRM.G.markers.route[i].hint;
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
 	
 	OSRM.G.markers.route[0].setPosition( positions[0] );
 	OSRM.G.markers.route[OSRM.G.markers.route.length-1].setPosition( positions[positions.length-1] );
 	for(var i=0; i<OSRM.G.via_points.length; i++)
		OSRM.G.markers.route[i+1].setPosition( new L.LatLng(OSRM.G.via_points[i][0], OSRM.G.via_points[i][1]) );

 	OSRM.Geocoder.updateAddress(OSRM.C.SOURCE_LABEL);
 	OSRM.Geocoder.updateAddress(OSRM.C.TARGET_LABEL);
}

};