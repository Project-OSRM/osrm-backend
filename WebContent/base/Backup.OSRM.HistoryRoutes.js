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

// OSRM route management (handles drawing of route geometry - current route, old route, unnamed route, highlight unnamed streets) 
// [this holds the route geometry]


OSRM.HistoryRoute = function() {
	// how to draw history routes and how many
	this._history_styles = [{dashed:false, color:'#FFFFFF', opacity:0.5, weight:5},
	                        {dashed:false, color:'#0000DD', opacity:0.45, weight:5},
	                        {dashed:false, color:'#0000BB', opacity:0.40, weight:5},
	                        {dashed:false, color:'#000099', opacity:0.35, weight:5},
	                        {dashed:false, color:'#000077', opacity:0.30, weight:5},
	                        {dashed:false, color:'#000055', opacity:0.25, weight:5},
	                        {dashed:false, color:'#000033', opacity:0.20, weight:5},
	                        {dashed:false, color:'#000011', opacity:0.15, weight:5},
	                        {dashed:false, color:'#FF0000', opacity:0.50, weight:5}	                              
	                        ];
	this._history_routes = this._history_styles.length;
	
	// actual history data
	this._history_route = [];
	this._history_data = [];
	for(var i=0, size=this._history_routes; i<size; i++) {
		this._history_route.push( new OSRM.SimpleRoute("current" , {dashed:false} ) );
		this._history_data[i] = [];
	}
	
	// helper functions bound to this
	this._initiate_redrawHistory = OSRM.bind(this, this._getRoute_RedrawHistory);
	this._callback_redrawHistory = OSRM.bind(this, this._showRoute_RedrawHistory);
};
OSRM.extend( OSRM.HistoryRoute,{
	activate: function() {
		this.storeHistoryRoute = this._storeHistoryRoute;
		this.fetchHistoryRoute = this._fetchHistoryRoute;
		this.showHistoryRoutes = this._showHistoryRoutes;
		this.clearHistoryRoutes = this._clearHistoryRoutes;
		OSRM.G.map.on('zoomend', this._initiate_redrawHistory );
		
		this.storeHistoryRoute();
	},
	deactivate: function() {
		this.clearHistoryRoutes();
		
		this.storeHistoryRoute = this.empty;
		this.fetchHistoryRoute = this.empty;
		this.showHistoryRoutes = this.empty;
		this.clearHistoryRoutes = this.empty;
		OSRM.G.map.off('zoomend', this._initiate_redrawHistory );
	},
	
	// empty function
	empty: function() {},
	storeHistoryRoute: function() {},
	fetchHistoryRoute: function() {},
	showHistoryRoutes: function() {},
	clearHistoryRoutes: function() {},
	
	// qctual functions
	_storeHistoryRoute: function() {
		var route = OSRM.G.route;
		
		if(route.isShown() && route.isRoute()) {
			console.log("store");			
			// store new route and positions in staging spot
			this._history_route[0].setPositions( route.getPositions() );
			this._history_data[0] = [];
			var markers = OSRM.G.markers.route;
			for(var i=0,size=markers.length; i<size; i++) {
				var position = {
						lat:markers[i].getLat(),
						lng:markers[i].getLng(),
						hint:markers[i].hint
				};
				this._history_data[0].push(position);
			}
			this._history_data[0].checksum = OSRM.G.markers.checksum;
		}
	},
	_showHistoryRoutes: function() {
		console.log("show");
		for(var i=1,size=this._history_routes; i<size; i++) {
			this._history_route[i].setStyle( this._history_styles[i] );
			this._history_route[i].show();
		}
	},
	_clearHistoryRoutes: function() {
		for(var i=0,size=this._history_routes; i<size; i++) {
			this._history_route[i].hide();
			this._history_route[i].setPositions( [] );
			this._history_data[i] = [];
		}
	},
	_fetchHistoryRoute: function() {
		if( this._history_data[0].length == 0)
			return;
		if( this._equalRoute() )
			return;
		console.log("fetch");
		// move route and positions
		for(var i=this._history_routes-1; i>0; i--) {
			this._history_route[i].setPositions( this._history_route[i-1].getPositions() );
			this._history_data[i] = this._history_data[i-1];
		}
		// reset staging spot
		this._history_route[0].setPositions( [] );
		this._history_data[0] = [];
	},
	_equalRoute: function() {
		var lhs = OSRM.G.markers.route;
		var rhs = this._history_data[0];
		for(var i=0,size=Math.min(rhs.length,lhs.length); i<size; i++) {
			if( lhs[i].getLat() != rhs[i].lat || lhs[i].getLng() != rhs[i].lng) {
				console.log("different routes");
				return false;
			}
		}
		console.log("same routes");
		return true;
//	 	OSRM.G.markers.route[0].setPosition( positions[0] );
//	 	OSRM.G.markers.route[OSRM.G.markers.route.length-1].setPosition( positions[positions.length-1] );
//	 	for(var i=0; i<OSRM.G.via_points.length; i++)
//			OSRM.G.markers.route[i+1].setPosition( new L.LatLng(OSRM.G.via_points[i][0], OSRM.G.via_points[i][1]) );		
	},
	
	// redraw
	_showRoute_RedrawHistory: function(response, history_id) {
		if(!response)
			return;
		
		var positions = OSRM.RoutingGeometry._decode(response.route_geometry, 5);
		this._history_route[history_id].setPositions(positions);
		this._updateHints(response, history_id);
	},
	_getRoute_RedrawHistory: function() {
		for(var i=0,size=this._history_routes; i<size; i++)
			if( this._history_data[i].length > 0 ) {
				OSRM.JSONP.clear('history'+i);
				OSRM.JSONP.call(this._buildCall(i)+'&instructions=false', this._callback_redrawHistory, OSRM.JSONP.empty, OSRM.DEFAULTS.JSONP_TIMEOUT, 'history'+i, i);
			}
	},
	_buildCall: function(history_id) {
		var source = OSRM.DEFAULTS.HOST_ROUTING_URL;
		source += '?z=' + OSRM.G.map.getZoom() + '&output=json&jsonp=%jsonp&geomformat=cmp';
		var data = this._history_data[history_id];
		if(data.checksum)
			source += '&checksum=' + data.checksum;
		for(var i=0,size=data.length; i<size; i++) {
			source += '&loc='  + data[i].lat.toFixed(6) + ',' + data[i].lng.toFixed(6);
			if( data[i].hint)
				source += '&hint=' + data[i].hint;
		}
		return source;
	},
	_updateHints: function(response, history_id) {
		var data = this._history_data[history_id];
		data.checksum = response.hint_data.checksum;
		var hint_locations = response.hint_data.locations;
		for(var i=0; i<hint_locations.length; i++)
			data[i].hint = hint_locations[i];		
	}
});