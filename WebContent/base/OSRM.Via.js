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

// OSRM via marker routines
// [find correct position for a via marker]


OSRM.Via = {
		
// find route segment of current route geometry that is closest to the new via node (marked by index of its endpoint)
_findNearestRouteSegment: function( new_via ) {
	var min_dist = Number.MAX_VALUE;
	var min_index = undefined;

	var p = OSRM.G.map.latLngToLayerPoint( new_via );
	var positions = OSRM.G.route.getPoints();
	for(var i=1; i<positions.length; i++) {
		var _sqDist = L.LineUtil._sqClosestPointOnSegment(p, positions[i-1], positions[i], true);
		if( _sqDist < min_dist) {
			min_dist = _sqDist;
			min_index = i;
		}
	}

	return min_index;
},


// find the correct index among all via nodes to insert the new via node, and insert it  
findViaIndex: function( new_via_position ) {
	// find route segment that is closest to click position (marked by last index)
	var nearest_index = OSRM.Via._findNearestRouteSegment( new_via_position );

	// find correct index to insert new via node
	var via_points = OSRM.G.response.via_points;
	var new_via_index = via_points.length-2;
	var via_index = Array();
	for(var i=1; i<via_points.length-1; i++) {
		via_index[i-1] = OSRM.Via._findNearestRouteSegment( new L.LatLng(via_points[i][0], via_points[i][1]) );
		if(via_index[i-1] > nearest_index) {
			new_via_index = i-1;
			break;
		}
	}

	// add via node
	return new_via_index;
},


//function that draws a drag marker
dragTimer: new Date(),

drawDragMarker: function(event) {
	if( OSRM.G.route.isShown() == false)
		return;
	if( OSRM.G.dragging == true )
		return;
	
	// throttle computation
	if( (new Date() - OSRM.Via.dragTimer) < 25 )
		return;
	OSRM.Via.dragTimer = new Date();
	
	// get distance to route
	var minpoint = OSRM.G.route._current_route.route.closestLayerPoint( event.layerPoint );
	var min_dist = minpoint ? minpoint.distance : 1000;
	
	// get distance to markers
	var mouse = event.latlng;
	for(var i=0, size=OSRM.G.markers.route.length; i<size; i++) {
		if(OSRM.G.markers.route[i].label=='drag')
			continue;
		var position = OSRM.G.markers.route[i].getPosition();
		var dist = OSRM.G.map.project(mouse).distanceTo(OSRM.G.map.project(position));
		if( dist < 20 )
			min_dist = 1000;
	}
	
	// check whether mouse is over another marker
	var pos = OSRM.G.map.layerPointToContainerPoint(event.layerPoint);
	var obj = document.elementFromPoint(pos.x,pos.y);
	for(var i=0, size=OSRM.G.markers.route.length; i<size; i++) {
		if(OSRM.G.markers.route[i].label=='drag')
			continue;
		if( obj == OSRM.G.markers.route[i].marker._icon)
			min_dist = 1000;
	}
	
	// special care for highlight marker
	if( OSRM.G.markers.highlight.isShown() ) {
		if( OSRM.G.map.project(mouse).distanceTo(OSRM.G.map.project( OSRM.G.markers.highlight.getPosition() ) ) < 20 )
			min_dist = 1000;
		else if( obj == OSRM.G.markers.highlight.marker._icon)
			min_dist = 1000;
	}
	
	if( min_dist < 20) {
		OSRM.G.markers.dragger.setPosition( OSRM.G.map.layerPointToLatLng(minpoint) );
		OSRM.G.markers.dragger.show();
	} else
		OSRM.G.markers.dragger.hide();
}

};