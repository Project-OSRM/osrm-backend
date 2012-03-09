var via_points;

function findNearestRouteSegment( new_via ) {
	var min_dist = Number.MAX_VALUE;
	var min_index = undefined;

	var positions = my_route.getPositions();
	for(var i=0; i<positions.length-1; i++) {
		var dist = dotLineLength( new_via.lng, new_via.lat, positions[i].lng, positions[i].lat, positions[i+1].lng, positions[i+1].lat, true);
		if( dist < min_dist) {
			min_dist = dist;
			min_index = i+1;
		}
	}

	return min_index;
}


function findViaPosition( new_via_position ) {
	// find route segment that is closest to click position (marked by last index)
	var nearest_index = findNearestRouteSegment( new_via_position );

	// find correct index to insert new via node
	var new_via_index = via_points.length;
	var via_index = Array();
	for(var i=0; i<via_points.length; i++) {
		via_index[i] = findNearestRouteSegment( new L.LatLng(via_points[i][0], via_points[i][1]) );
		if(via_index[i] > nearest_index) {
			new_via_index = i;
			break;
		}
	}

	// add via node
	var index = my_markers.setVia(new_via_index, new_via_position);
	my_markers.route[index].show();
	
	getRoute(OSRM.FULL_DESCRIPTION);

	return new_via_index;
}