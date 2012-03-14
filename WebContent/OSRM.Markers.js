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

// OSRM markers 
// [base marker class, derived highlight marker and route marker classes, marker management]  


// base marker class (wraps Leaflet markers)
OSRM.Marker = function( label, style, position ) {
	this.label = label ? label : "marker";
	this.position = position ? position : new L.LatLng(0,0);

	this.marker = new L.MouseMarker( this.position, style );
	this.marker.parent = this;
	
	this.shown = false;
	this.hint = undefined;
};
OSRM.extend( OSRM.Marker,{
show: function() {
	map.addLayer(this.marker);
	this.shown = true;
},
hide: function() {
	map.removeLayer(this.marker);
	this.shown = false;
},
setPosition: function( position ) {
	this.position = position;
	this.marker.setLatLng( position );
	this.hint = undefined;	
},
getPosition: function() {
	return this.position;
},
getLat: function() {
	return this.position.lat;
},
getLng: function() {
	return this.position.lng;
},
isShown: function() {
	return this.shown;
},
centerView: function(zooming) {
	var zoom = OSRM.DEFAULTS.ZOOM_LEVEL;
	if( zooming == false )
		zoom = map.getZoom();
	//map.setView( new L.LatLng( this.position.lat, this.position.lng-0.02), zoom);		// dirty hack
	map.setView( new L.LatLng( this.position.lat, this.position.lng), zoom);
},
toString: function() {
	return "OSRM.Marker: \""+this.label+"\", "+this.position+")";
},
});


// highlight marker class (cannot be dragged)
OSRM.HighlightMarker = function( label, style, position) {
	OSRM.HighlightMarker.prototype.base.constructor.apply( this, arguments );
	this.label = label ? label : "highlight_marker";
};
OSRM.inheritFrom( OSRM.HighlightMarker, OSRM.Marker );
OSRM.extend( OSRM.HighlightMarker, {
toString: function() {
	return "OSRM.HighlightMarker: \""+this.label+"\", "+this.position+")";
},
});


// route marker class (draggable, invokes route drawing routines) 
OSRM.RouteMarker = function ( label, style, position ) {
	OSRM.RouteMarker.prototype.base.constructor.apply( this, arguments );
	this.label = label ? label : "route_marker";

 	this.marker.on( 'click', this.onClick );
 	this.marker.on( 'drag', this.onDrag );
 	this.marker.on( 'dragstart', this.onDragStart );
 	this.marker.on( 'dragend', this.onDragEnd );
};
OSRM.inheritFrom( OSRM.RouteMarker, OSRM.Marker );
OSRM.extend( OSRM.RouteMarker, {
onClick: function(e) {
	for( var i=0; i<my_markers.route.length; i++) {
		if( my_markers.route[i].marker === this ) {
			my_markers.removeMarker( i );
			break;
		}
	}
	
	getRoute(OSRM.FULL_DESCRIPTION);
	my_markers.highlight.hide();
},
onDrag: function(e) {
	this.parent.setPosition( e.target.getLatLng() );
	if(OSRM.dragging == true)								// TODO: hack that deals with drag events after dragend event
		getRoute(OSRM.NO_DESCRIPTION);
	else
		getRoute(OSRM.FULL_DESCRIPTION);
	
	updateLocation( this.parent.label );
},
onDragStart: function(e) {
	OSRM.dragging = true;
	
	// hack to store id of dragged marker
	for( var i=0; i<my_markers.route.length; i++)
		if( my_markers.route[i].marker === this ) {
			OSRM.dragid = i;
			break;
		}			
	
	my_markers.highlight.hide();	
	if (my_route.isShown()) {
		my_route.showOldRoute();
	}
	
	updateLocation( this.parent.label );	
},
onDragEnd: function(e) {
	getRoute(OSRM.FULL_DESCRIPTION);
	if (my_route.isShown()) {
		my_route.hideOldRoute();
		my_route.hideUnnamedRoute();
	}
	OSRM.dragging = false;
	
	updateLocation( this.parent.label );
},
toString: function() {
	return "OSRM.RouteMarker: \""+this.label+"\", "+this.position+")";
},
});


// marker management class (all route markers should only be set and deleted with these routines!)
// [this holds the vital information of the route]
OSRM.Markers = function() {
	this.route = new Array();
	this.highlight = new OSRM.HighlightMarker("highlight", {draggable:false,icon:OSRM.icons['marker-highlight']});;
};
OSRM.extend( OSRM.Markers,{
removeAll: function() {
	for(var i=0; i<this.route.length;i++)
		this.route[i].hide();
	this.route.splice(0, this.route.length);
},
removeVias: function() {
	// assert correct route array s - v - t
	for(var i=1; i<this.route.length-1;i++)
		this.route[i].hide();
	this.route.splice(1, this.route.length-2);
},
setSource: function(position) {
	// source node is always first node
	if( this.route[0] && this.route[0].label == OSRM.SOURCE_MARKER_LABEL )
		this.route[0].setPosition(position);
	else
		this.route.splice(0,0, new OSRM.RouteMarker("source", {draggable:true,icon:OSRM.icons['marker-source']}, position));
	return 0;	
},
setTarget: function(position) {
	// target node is always last node
	if( this.route[this.route.length-1] && this.route[ this.route.length-1 ].label == OSRM.TARGET_MARKER_LABEL )
		this.route[this.route.length-1].setPosition(position);
	else
		this.route.splice( this.route.length,0, new OSRM.RouteMarker("target", {draggable:true,icon:OSRM.icons['marker-target']}, position));
	return this.route.length-1;
},
setVia: function(id, position) {
	// via nodes only between source and target nodes
	if( this.route.length<2 || id > this.route.length-2 )
		return -1;
	
	this.route.splice(id+1,0, new OSRM.RouteMarker("via", {draggable:true,icon:OSRM.icons['marker-via']}, position));
	return id+1;
},
removeMarker: function(id) {
	if( id >= this.route.length )
		return;
	
	// also remove vias if source or target are removed
	if( id==0 && this.route[0].label == OSRM.SOURCE_MARKER_LABEL )
		this.removeVias();
	else if( id == this.route.length-1 && this.route[ this.route.length-1 ].label == OSRM.TARGET_MARKER_LABEL ) {
		this.removeVias();
		id = this.route.length-1;
	}
	
	this.route[id].hide();
	this.route.splice(id, 1);
}
});