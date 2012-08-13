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


OSRM.Route = function() {
	this._current_route		= new OSRM.SimpleRoute("current" , {dashArray:""} );
	this._alternative_route	= new OSRM.SimpleRoute("alternative" , {dashArray:""} );
	this._old_route			= new OSRM.SimpleRoute("old", {color:"#123", dashArray:""} );
	this._unnamed_route		= new OSRM.MultiRoute("unnamed");
	
	this._current_route_style	= {color:'#0033FF', weight:5, dashArray:""};
	this._current_noroute_style	= {color:'#222222', weight:2, dashArray:"8,6"};
	this._old_route_style	= {color:'#112233', weight:5, dashArray:""};
	this._old_noroute_style	= {color:'#000000', weight:2, dashArray:"8,6"};
	this._unnamed_route_style = {color:'#FF00FF', weight:10, dashArray:""};
	this._old_unnamed_route_style = {color:'#990099', weight:10, dashArray:""};
	this._alternative_route_style	= {color:'#770033', weight:5, opacity:0.6, dashArray:""};
	
	this._noroute = OSRM.Route.ROUTE;
	this._history = new OSRM.HistoryRoute();
};
OSRM.Route.NOROUTE = true;
OSRM.Route.ROUTE = false;
OSRM.extend( OSRM.Route,{

	// show/hide route
	showRoute: function(positions, noroute) {
		this._noroute = noroute;
		this._current_route.setPositions( positions );
		if ( this._noroute == OSRM.Route.NOROUTE )
			this._current_route.setStyle( this._current_noroute_style );
		else
			this._current_route.setStyle( this._current_route_style );
		this._current_route.show();
		//this._raiseUnnamedRoute();

		this._history.fetchHistoryRoute();
		this._history.showHistoryRoutes();		
		this._history.storeHistoryRoute();		
	},
	hideRoute: function() {
		this._current_route.hide();
		this._unnamed_route.hide();
		
		this._history.fetchHistoryRoute();
		this._history.showHistoryRoutes();		
		// deactivate GUI features that need a route
		OSRM.GUI.deactivateRouteFeatures();
	},
	
	// show/hide highlighting for unnamed routes
	showUnnamedRoute: function(positions) {
		this._unnamed_route.clearRoutes();
		for(var i=0; i<positions.length; i++) {
			this._unnamed_route.addRoute(positions[i]);	
		}
		this._unnamed_route.setStyle( this._unnamed_route_style );
		this._unnamed_route.show();
	},
	hideUnnamedRoute: function() {
		this._unnamed_route.hide();
	},
	// TODO: hack to put unnamed_route above old_route -> easier way in will be available Leaflet 0.4	
	_raiseUnnamedRoute: function() {
		if(this._unnamed_route.isShown()) {
			this._unnamed_route.hide();
			this._unnamed_route.show();
		}		
	},
	
	// show/hide previous route as shadow
	showOldRoute: function() {
		this._old_route.setPositions( this._current_route.getPositions() );
		if ( this._noroute == OSRM.Route.NOROUTE)
			this._old_route.setStyle( this._old_noroute_style );
		else
			this._old_route.setStyle( this._old_route_style );
		this._old_route.show();
		this._raiseUnnamedRoute();
		// change color of unnamed route highlighting - no separate object as dragged route does not have unnamed route highlighting
		this._unnamed_route.setStyle( this._old_unnamed_route_style );
	},
	hideOldRoute: function() {
		this._old_route.hide();
	},
	
	// show/hide alternative route
	showAlternativeRoute: function(positions) {
		this._alternative_route.setPositions( positions );
		this._alternative_route.setStyle( this._alternative_route_style );
		this._alternative_route.show();		
	},
	hideAlternativeRoute: function() {
		this._alternative_route.hide();
	},
	
	// query routines
	isShown: function() {
		return this._current_route.isShown();
	},
	isRoute: function() {
		return !(this._noroute);
	},	
	getPositions: function() {
		return this._current_route.getPositions();
	},
	getPoints: function() {
		return this._current_route.getPoints();
	},
	
	// helper routines
	reset: function() {
		this.hideRoute();
		this._old_route.hide();
		this._noroute = OSRM.Route.ROUTE;
		this._history.clearHistoryRoutes();
	},
	fire: function(type,event) {
		this._current_route.route.fire(type,event);
	},
	centerView: function() {
		this._current_route.centerView();
	},	
	
	// handle history routes
	activateHistoryRoutes: function() {
		this._history.activate();
	},
	deactivateHistoryRoutes: function() {
		this._history.deactivate();
	}	
});
