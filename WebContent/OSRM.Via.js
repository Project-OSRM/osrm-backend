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

// store location of via points returned by server
OSRM.GLOBALS.via_points = null;


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
findViaPosition: function( new_via_position ) {
	// find route segment that is closest to click position (marked by last index)
	var nearest_index = OSRM.Via._findNearestRouteSegment( new_via_position );

	// find correct index to insert new via node
	var new_via_index = OSRM.G.via_points.length;
	var via_index = Array();
	for(var i=0; i<OSRM.G.via_points.length; i++) {
		via_index[i] = OSRM.Via._findNearestRouteSegment( new L.LatLng(OSRM.G.via_points[i][0], OSRM.G.via_points[i][1]) );
		if(via_index[i] > nearest_index) {
			new_via_index = i;
			break;
		}
	}

	// add via node
	var index = OSRM.G.markers.setVia(new_via_index, new_via_position);
	OSRM.G.markers.route[index].show();
	
	OSRM.Routing.getRoute(OSRM.C.FULL_DESCRIPTION);

	return new_via_index;
}

};