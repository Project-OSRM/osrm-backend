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
	// style and count of history routes
	this._history_styles = [{color:'#FFFFFF', opacity:0.5, weight:5, dashArray:""},
	                        {color:'#0000DD', opacity:0.45, weight:5, dashArray:""},
	                        {color:'#0000BB', opacity:0.40, weight:5, dashArray:""},
	                        {color:'#000099', opacity:0.35, weight:5, dashArray:""},
	                        {color:'#000077', opacity:0.30, weight:5, dashArray:""},
	                        {color:'#000055', opacity:0.25, weight:5, dashArray:""},
	                        {color:'#000033', opacity:0.20, weight:5, dashArray:""},
	                        {color:'#000011', opacity:0.15, weight:5, dashArray:""},
	                        {color:'#000000', opacity:0.10, weight:5, dashArray:""}
	                        ];
	this._history_length = this._history_styles.length;
	
	// actual history data
	this._history = [];
	for(var i=0, size=this._history_length; i<size; i++) {
		var history = {};
		history.route = new OSRM.SimpleRoute("current" , {dashArray:""} );
		history.markers = [];
		history.checksum = null;
		this._history.push(history);
	}
	
	// helper functions bound to this
	this._initiate_redrawHistory = OSRM.bind(this, this._getRoute_RedrawHistory);
	this._callback_redrawHistory = OSRM.bind(this, this._showRoute_RedrawHistory);
};
OSRM.extend( OSRM.HistoryRoute,{
	// switch history routes on/off
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
	
	// actual functions
	_storeHistoryRoute: function() {
		var route = OSRM.G.route;
		if( !route.isShown() || !route.isRoute() )
			return;
			
		// store current route in staging spot
		var hint_data = OSRM.G.response.hint_data;
		this._history[0].route.setPositions( route.getPositions() );
		this._history[0].checksum = hint_data.checksum;
		this._history[0].markers = [];

		var markers = this._getCurrentMarkers();
		for(var i=0,size=markers.length; i<size; i++) {
			var position = { lat:markers[i].lat, lng:markers[i].lng, hint:hint_data.locations[i] };
			this._history[0].markers.push(position);
		}
	},
	_fetchHistoryRoute: function() {
		if( this._history[0].markers.length == 0 )
			return;
		if( OSRM.G.route.isShown() && this._equalMarkers(this._history[0].markers, this._getCurrentMarkers()) )
			return;
		if( this._equalMarkers(this._history[0].markers, this._history[1].markers) )
			return;		

		// move all routes down one position
		for(var i=this._history_length-1; i>0; i--) {
			this._history[i].route.setPositions( this._history[i-1].route.getPositions() ); // copying positions quicker than creating new route!
			this._history[i].markers = this._history[i-1].markers;
			this._history[i].checksum = this._history[i-1].checksum;
		}		
		// reset staging spot
		this._history[0].route.setPositions( [] );
		this._history[0].markers = [];
		this._history[0].checksum = null;
	},	
	_showHistoryRoutes: function() {
		for(var i=1,size=this._history_length; i<size; i++) {
			this._history[i].route.setStyle( this._history_styles[i] );
			this._history[i].route.show();
			OSRM.G.route.hideOldRoute();
		}
	},
	_clearHistoryRoutes: function() {
		for(var i=0,size=this._history_length; i<size; i++) {
			this._history[i].route.hide();
			this._history[i].route.setPositions( [] );
			this._history[i].markers = [];
			this._history[i].checksum = null;
		}
	},
	
	// get positions of current markers (data of jsonp response used, as not all data structures up-to-date!)
	_getCurrentMarkers: function() {
		var route = [];
		
		var positions = OSRM.G.route.getPositions();
		if(positions.length == 0)
			return route;
		
		for(var i=0; i<OSRM.G.response.via_points.length; i++)
			route.push( {lat:OSRM.G.response.via_points[i][0], lng:OSRM.G.response.via_points[i][1]} );
		return route;
	},
	
	// check if two routes are equivalent by checking their markers
	_equalMarkers: function(lhs, rhs) {
		if(lhs.length != rhs.length)
			return false;
		for(var i=0,size=lhs.length; i<size; i++) {
			if( lhs[i].lat.toFixed(5) != rhs[i].lat.toFixed(5) || lhs[i].lng.toFixed(5) != rhs[i].lng.toFixed(5) )
				return false;
		}
		return true;
	},
	
	// requery history routes
	_showRoute_RedrawHistory: function(response, history_id) {
		if(!response)
			return;
		
		var positions = OSRM.RoutingGeometry._decode(response.route_geometry, 5);
		this._history[history_id].route.setPositions(positions);
		this._updateHints(response, history_id);
	},
	_getRoute_RedrawHistory: function() {
		for(var i=0,size=this._history_length; i<size; i++)
			if( this._history[i].markers.length > 0 ) {
				OSRM.JSONP.clear('history'+i);
				OSRM.JSONP.call(this._buildCall(i)+'&instructions=false', this._callback_redrawHistory, OSRM.JSONP.empty, OSRM.DEFAULTS.JSONP_TIMEOUT, 'history'+i, i);
			}
	},
	_buildCall: function(history_id) {
		var source = OSRM.G.active_routing_server_url;
		source += '?z=' + OSRM.G.map.getZoom() + '&output=json&jsonp=%jsonp';
		
		if(this._history[history_id].checksum)
			source += '&checksum=' + this._history[history_id].checksum;
		
		var history_markers = this._history[history_id].markers; 
		for(var i=0,size=history_markers.length; i<size; i++) {
			source += '&loc='  + history_markers[i].lat.toFixed(6) + ',' + history_markers[i].lng.toFixed(6);
			if( history_markers[i].hint )
				source += '&hint=' + history_markers[i].hint;
		}
		return source;
	},
	_updateHints: function(response, history_id) {
		this._history[history_id].checksum = response.hint_data.checksum;
		
		var hints = response.hint_data.locations;
		for(var i=0; i<hints.length; i++)
			this._history[history_id].markers[i].hint = hints[i];		
	}
});
