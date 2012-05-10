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
	this._current_route	= new OSRM.SimpleRoute("current" , {dashed:false} );
	this._old_route		= new OSRM.SimpleRoute("old", {dashed:false,color:"#123"} );
	this._unnamed_route	= new OSRM.MultiRoute("unnamed");
	
	this._current_route_style	= {dashed:false,color:'#0033FF', weight:5};
	this._current_noroute_style	= {dashed:true, color:'#222222', weight:2};
	this._old_route_style	= {dashed:false,color:'#112233', weight:5};
	this._old_noroute_style	= {dashed:true, color:'#000000', weight:2};
	this._unnamed_route_style = {dashed:false, color:'#FF00FF', weight:10};
	this._old_unnamed_route_style = {dashed:false, color:'#990099', weight:10};
	
	this._noroute = OSRM.Route.ROUTE;	
	
	this._route_history_styles = [{dashed:false, color:'#FF0000', weight:5},
	                              {dashed:false, color:'#00FF00', weight:5},
	                              {dashed:false, color:'#0000FF', weight:5},
	                              {dashed:false, color:'#FF00FF', weight:5},
	                              {dashed:false, color:'#00FFFF', weight:5},
	                              {dashed:false, color:'#770000', weight:5},
	                              {dashed:false, color:'#007700', weight:5},
	                              {dashed:false, color:'#000077', weight:5},
	                              {dashed:false, color:'#770077', weight:5},
	                              {dashed:false, color:'#007777', weight:5}	                              
	                              ];
	this._route_history = [];
	for(var i=0, size=this._route_history_styles.length; i<size; i++)
		this._route_history.push( new OSRM.SimpleRoute("current" , {dashed:false} ) );
};
OSRM.Route.NOROUTE = true;
OSRM.Route.ROUTE = false;
OSRM.extend( OSRM.Route,{
	
	showRoute: function(positions, noroute) {
		this.showHistoryRoutes();
		this._noroute = noroute;
		this._current_route.setPositions( positions );
		if ( this._noroute == OSRM.Route.NOROUTE )
			this._current_route.setStyle( this._current_noroute_style );
		else
			this._current_route.setStyle( this._current_route_style );
		this._current_route.show();
		//this._raiseUnnamedRoute();
	},
	hideRoute: function() {
		this.showHistoryRoutes();		
		this._current_route.hide();
		this._unnamed_route.hide();
		// deactivate printing
		OSRM.Printing.deactivate();		
	},
	hideAll: function() {
		this.hideRoute();
		this._old_route.hide();
		this._noroute = OSRM.Route.ROUTE;
		this.clearHistoryRoutes();
	},
	
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
	fire: function(type,event) {
		this._current_route.route.fire(type,event);
	},
	centerView: function() {
		this._current_route.centerView();
	},
	
	// history route handling
	storeHistoryRoute: function() {
		if( document.getElementById('option-show-previous-routes').checked == false)
			return;
		if(this.isShown() && this.isRoute()) {
			for(var i=this._route_history.length-1; i>0; i--)
				this._route_history[i].setPositions( this._route_history[i-1].getPositions() );
			this._route_history[0].setPositions( this._current_route.getPositions() );
		}
	},
	showHistoryRoutes: function() {
		if( document.getElementById('option-show-previous-routes').checked == false)
			return;
		for(var i=1,size=this._route_history.length; i<size; i++) {
			this._route_history[i].setStyle( this._route_history_styles[i] );
			this._route_history[i].show();
		}
	},
	clearHistoryRoutes: function() {
		for(var i=0,size=this._route_history.length; i<size; i++) {
			this._route_history[i].hide();
			this._route_history[i].setPositions( [] );
		}
	}	
});
