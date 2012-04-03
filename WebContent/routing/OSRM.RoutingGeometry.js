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

// OSRM routing geometry
// [renders routing geometry]


OSRM.RoutingGeometry = {

// show route geometry - if there is a route
show: function(response) {
	var geometry = OSRM.RoutingGeometry._decode(response.route_geometry, 5);

	var positions = [];
	for( var i=0, size=geometry.length; i < size; i++)
		positions.push( new L.LatLng(geometry[i][0], geometry[i][1]) );
	
	OSRM.G.route.showRoute(positions, OSRM.Route.ROUTE);
},

//show route geometry - if there is no route
showNA: function() {
	var positions = [];
	for(var i=0, size=OSRM.G.markers.route.length; i<size; i++)
			positions.push( OSRM.G.markers.route[i].getPosition() );

	OSRM.G.route.showRoute(positions, OSRM.Route.NOROUTE);
},

//decode compressed route geometry
_decode: function(encoded, precision) {
	precision = Math.pow(10, -precision);
	var len = encoded.length, index=0, lat=0, lng = 0, array = [];
	while (index < len) {
		var b, shift = 0, result = 0;
		do {
			b = encoded.charCodeAt(index++) - 63;
			result |= (b & 0x1f) << shift;
			shift += 5;
		} while (b >= 0x20);
		var dlat = ((result & 1) ? ~(result >> 1) : (result >> 1));
		lat += dlat;
		shift = 0;
		result = 0;
		do {
			b = encoded.charCodeAt(index++) - 63;
			result |= (b & 0x1f) << shift;
			shift += 5;
		} while (b >= 0x20);
		var dlng = ((result & 1) ? ~(result >> 1) : (result >> 1));
		lng += dlng;
		array.push([lat * precision, lng * precision]);
	}
	return array;
}

};