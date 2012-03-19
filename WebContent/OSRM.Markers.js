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
	this.hint = null;
};
OSRM.extend( OSRM.Marker,{
show: function() {
	OSRM.G.map.addLayer(this.marker);
	this.shown = true;
},
hide: function() {
	OSRM.G.map.removeLayer(this.marker);
	this.shown = false;
},
setPosition: function( position ) {
	this.position = position;
	this.marker.setLatLng( position );
	this.hint = null;	
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
centerView: function(zoom) {
	if( zoom == undefined )
		zoom = OSRM.DEFAULTS.ZOOM_LEVEL;
	OSRM.G.map.setView( new L.LatLng( this.position.lat, this.position.lng), zoom);
},
toString: function() {
	return "OSRM.Marker: \""+this.label+"\", "+this.position+")";
}
});


// highlight marker class (cannot be dragged)
OSRM.HighlightMarker = function( label, style, position) {
	OSRM.HighlightMarker.prototype.base.constructor.apply( this, arguments );
	this.label = label ? label : "highlight_marker";
	
 	this.marker.on( 'click', this.onClick );	
};
OSRM.inheritFrom( OSRM.HighlightMarker, OSRM.Marker );
OSRM.extend( OSRM.HighlightMarker, {
toString: function() {
	return "OSRM.HighlightMarker: \""+this.label+"\", "+this.position+")";
},
onClick: function(e) {
	this.parent.hide();
}
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
	for( var i=0; i<OSRM.G.markers.route.length; i++) {
		if( OSRM.G.markers.route[i].marker === this ) {
			OSRM.G.markers.removeMarker( i );
			break;
		}
	}
	
	getRoute(OSRM.C.FULL_DESCRIPTION);
	OSRM.G.markers.highlight.hide();
},
onDrag: function(e) {
	this.parent.setPosition( e.target.getLatLng() );
	getRoute(OSRM.C.NO_DESCRIPTION);
	updateLocation( this.parent.label );
},
onDragStart: function(e) {
	OSRM.G.dragging = true;
	
	// store id of dragged marker
	for( var i=0; i<OSRM.G.markers.route.length; i++)
		if( OSRM.G.markers.route[i].marker === this ) {
			OSRM.G.dragid = i;
			break;
		}			
	
	OSRM.G.markers.highlight.hide();	
	if (OSRM.G.route.isShown())
		OSRM.G.route.showOldRoute();
},
onDragEnd: function(e) {
	OSRM.G.dragging = false;
	this.parent.setPosition( e.target.getLatLng() );	
	getRoute(OSRM.C.FULL_DESCRIPTION);
	if (OSRM.G.route.isShown()) {
		OSRM.G.route.hideOldRoute();
		OSRM.G.route.hideUnnamedRoute();
	}
	
	if(OSRM.G.route.isShown()==false)
		updateAddress(this.parent.label);
},
toString: function() {
	return "OSRM.RouteMarker: \""+this.label+"\", "+this.position+")";
}
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
	if( this.route[0] && this.route[0].label == OSRM.C.SOURCE_LABEL )
		this.route[0].setPosition(position);
	else
		this.route.splice(0,0, new OSRM.RouteMarker(OSRM.C.SOURCE_LABEL, {draggable:true,icon:OSRM.icons['marker-source']}, position));
	return 0;	
},
setTarget: function(position) {
	// target node is always last node
	if( this.route[this.route.length-1] && this.route[ this.route.length-1 ].label == OSRM.C.TARGET_LABEL )
		this.route[this.route.length-1].setPosition(position);
	else
		this.route.splice( this.route.length,0, new OSRM.RouteMarker(OSRM.C.TARGET_LABEL, {draggable:true,icon:OSRM.icons['marker-target']}, position));
	return this.route.length-1;
},
setVia: function(id, position) {
	// via nodes only between source and target nodes
	if( this.route.length<2 || id > this.route.length-2 )
		return -1;
	
	this.route.splice(id+1,0, new OSRM.RouteMarker(OSRM.C.VIA_LABEL, {draggable:true,icon:OSRM.icons['marker-via']}, position));
	return id+1;
},
removeMarker: function(id) {
	if( id >= this.route.length )
		return;
	
	// also remove vias if source or target are removed
	if( id==0 && this.route[0].label == OSRM.C.SOURCE_LABEL ) {
		this.removeVias();
		document.getElementById('input-source-name').value = "";
	} else if( id == this.route.length-1 && this.route[ this.route.length-1 ].label == OSRM.C.TARGET_LABEL ) {
		this.removeVias();
		id = this.route.length-1;
		document.getElementById('input-target-name').value = "";
	}
	
	this.route[id].hide();
	this.route.splice(id, 1);
},
hasSource: function() {
	if( OSRM.G.markers.route[0] && OSRM.G.markers.route[0].label == OSRM.C.SOURCE_LABEL )
		return true;
	return false;
},
hasTarget: function() {
	if( OSRM.G.markers.route[OSRM.G.markers.route.length-1] && OSRM.G.markers.route[OSRM.G.markers.route.length-1].label == OSRM.C.TARGET_LABEL )
		return true;
	return false;
}
});
