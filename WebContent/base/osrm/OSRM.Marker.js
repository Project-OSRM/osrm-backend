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
// [base marker class, derived highlight marker and route marker classes]  


// base marker class (wraps Leaflet markers)
OSRM.Marker = function( label, style, position ) {
	this.label = label ? label : "marker";
	this.position = position ? position : new L.LatLng(0,0);

	this.marker = new L.LabelMarker( this.position, style );
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
	
	// revert highlighted description
	if( this.label == "highlight" )
	if( this.description ) {
		var desc = document.getElementById("description-"+this.description);
		desc &&	(desc.className = "description-body-item");
		this.description = null;
	}
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
	OSRM.G.map.setViewUI( this.position, zoom );
},
toString: function() {
	return "OSRM.Marker: \""+this.label+"\", "+this.position+")";
}
});


// route marker class (draggable, invokes route drawing routines) 
OSRM.RouteMarker = function ( label, style, position ) {
	style.baseicon = style.icon;
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
	
	OSRM.Routing.getRoute();
	OSRM.G.markers.highlight.hide();
	OSRM.G.markers.dragger.hide();
},
onDrag: function(e) {
	this.parent.setPosition( e.target.getLatLng() );
	if(OSRM.G.markers.route.length>1)
		OSRM.Routing.getRoute_Dragging();
	OSRM.Geocoder.updateLocation( this.parent.label );
},
onDragStart: function(e) {
	OSRM.GUI.deactivateTooltip( "DRAGGING" );	
	OSRM.G.dragging = true;
	this.changeIcon(this.options.dragicon);
	
	// store id of dragged marker
	for( var i=0; i<OSRM.G.markers.route.length; i++)
		if( OSRM.G.markers.route[i].marker === this ) {
			OSRM.G.dragid = i;
			break;
		}
	
	if( this.parent != OSRM.G.markers.highlight)
		OSRM.G.markers.highlight.hide();
	if( this.parent != OSRM.G.markers.dragger)
		OSRM.G.markers.dragger.hide();
	if (OSRM.G.route.isShown())
		OSRM.G.route.showOldRoute();
},
onDragEnd: function(e) {
	OSRM.G.dragging = false;
	this.changeIcon(this.options.baseicon);
	
	this.parent.setPosition( e.target.getLatLng() );	
	if (OSRM.G.route.isShown()) {
		OSRM.Routing.getRoute();
		OSRM.G.route.hideOldRoute();
		OSRM.G.route.hideUnnamedRoute();
	} else {
		OSRM.Geocoder.updateAddress(this.parent.label);
		OSRM.GUI.clearResults();
	}
},
toString: function() {
	return "OSRM.RouteMarker: \""+this.label+"\", "+this.position+")";
}
});


//drag marker class (draggable, invokes route drawing routines) 
OSRM.DragMarker = function ( label, style, position ) {
	OSRM.DragMarker.prototype.base.constructor.apply( this, arguments );
	this.label = label ? label : "drag_marker";
};
OSRM.inheritFrom( OSRM.DragMarker, OSRM.RouteMarker );
OSRM.extend( OSRM.DragMarker, {
onClick: function(e) {
	if( this.parent != OSRM.G.markers.dragger)
		this.parent.hide();
},
onDragStart: function(e) {
	var new_via_index = OSRM.Via.findViaIndex( e.target.getLatLng() );
	OSRM.G.markers.route.splice(new_via_index+1,0, this.parent );

	OSRM.RouteMarker.prototype.onDragStart.call(this,e);
},
onDragEnd: function(e) {
	OSRM.G.markers.route[OSRM.G.dragid] = new OSRM.RouteMarker(OSRM.C.VIA_LABEL, {draggable:true,icon:OSRM.G.icons['marker-via'],dragicon:OSRM.G.icons['marker-via-drag']}, e.target.getLatLng() );
	OSRM.G.markers.route[OSRM.G.dragid].show();
	
	OSRM.RouteMarker.prototype.onDragEnd.call(this,e);
	this.parent.hide();
},
toString: function() {
	return "OSRM.DragMarker: \""+this.label+"\", "+this.position+")";
}
});
