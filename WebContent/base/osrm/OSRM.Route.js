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

// OSRM routes
// [drawing of all types of route geometry] 


// simple route class (wraps Leaflet Polyline)
OSRM.SimpleRoute = function (label, style) {
	this.label = (label ? label : "route");
	this.route = new L.Polyline( [], style );
	this.shown = false;
};
OSRM.extend( OSRM.SimpleRoute,{
show: function() {
	OSRM.G.map.addLayer(this.route);
	this.shown = true;
},
hide: function() {
	OSRM.G.map.removeLayer(this.route);
	this.shown = false;
},
isShown: function() {
	return this.shown;
},
getPoints: function() {
	return this.route._originalPoints;
},
getPositions: function() {
	return this.route.getLatLngs();
},
setPositions: function(positions) {
	this.route.setLatLngs( positions );
},
setStyle: function(style) {
	this.route.setStyle(style);
},
centerView: function() {
	var bounds = new L.LatLngBounds( this.getPositions() );
	OSRM.g.map.fitBoundsUI( bounds );
},
toString: function() {
	return "OSRM.Route("+ this.label + ", " + this.route.getLatLngs().length + " points)";
}
});


// multiroute class (wraps Leaflet LayerGroup to hold several disjoint routes)
OSRM.MultiRoute = function (label) {
	this.label = (label ? label : "multiroute");
	this.route = new L.LayerGroup();

	this.shown = false;
};
OSRM.extend( OSRM.MultiRoute,{
show: function() {
	OSRM.G.map.addLayer(this.route);
	this.shown = true;
},
hide: function() {
	OSRM.G.map.removeLayer(this.route);
	this.shown = false;
},
isShown: function() {
	return this.shown;
},
addRoute: function(positions) {
	var line = new L.Polyline( positions );
	line.on('click', function(e) { OSRM.G.route.fire('click',e); });
	this.route.addLayer( line );
},
clearRoutes: function() {
	this.route.clearLayers();
},
setStyle: function(style) {
	this.route.invoke('setStyle', style);
},
toString: function() {
	return "OSRM.MultiRoute("+ this.label + ")";
}
});
