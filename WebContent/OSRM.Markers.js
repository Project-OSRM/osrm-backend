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
},
onDrag: function(e) {
	this.parent.setPosition( e.target.getLatLng() );
	if(OSRM.G.markers.route.length>1)
		OSRM.Routing.getDragRoute();
	OSRM.Geocoder.updateLocation( this.parent.label );
},
onDragStart: function(e) {
	OSRM.G.dragging = true;
	this.switchIcon(this.options.dragicon);
	
	// store id of dragged marker
	for( var i=0; i<OSRM.G.markers.route.length; i++)
		if( OSRM.G.markers.route[i].marker === this ) {
			OSRM.G.dragid = i;
			break;
		}
	
	if( this.parent != OSRM.G.markers.highlight)
		OSRM.G.markers.highlight.hide();	
	if (OSRM.G.route.isShown())
		OSRM.G.route.showOldRoute();
},
onDragEnd: function(e) {
	OSRM.G.dragging = false;
	this.switchIcon(this.options.baseicon);
	
	this.parent.setPosition( e.target.getLatLng() );	
	OSRM.Routing.getRoute();
	if (OSRM.G.route.isShown()) {
		OSRM.G.route.hideOldRoute();
		OSRM.G.route.hideUnnamedRoute();
	}
	
	if(OSRM.G.route.isShown()==false)
		OSRM.Geocoder.updateAddress(this.parent.label);
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


// marker management class (all route markers should only be set and deleted with these routines!)
// [this holds the vital information of the route]
OSRM.Markers = function() {
	this.route = new Array();
	this.highlight = new OSRM.DragMarker("highlight", {draggable:true,icon:OSRM.G.icons['marker-highlight'],dragicon:OSRM.G.icons['marker-highlight-drag']});;
	this.dragger = new OSRM.DragMarker("drag", {draggable:true,icon:OSRM.G.icons['marker-drag'],dragicon:OSRM.G.icons['marker-drag']});;
};
OSRM.extend( OSRM.Markers,{
removeAll: function() {
	for(var i=0; i<this.route.length;i++)
		this.route[i].hide();
	this.route.splice(0, this.route.length);
	document.getElementById('delete-source-marker').style.visibility = "hidden";
	document.getElementById('delete-target-marker').style.visibility = "hidden";
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
		this.route.splice(0,0, new OSRM.RouteMarker(OSRM.C.SOURCE_LABEL, {draggable:true,icon:OSRM.G.icons['marker-source'],dragicon:OSRM.G.icons['marker-source-drag']}, position));
	document.getElementById('delete-source-marker').style.visibility = "visible";
	return 0;	
},
setTarget: function(position) {
	// target node is always last node
	if( this.route[this.route.length-1] && this.route[ this.route.length-1 ].label == OSRM.C.TARGET_LABEL )
		this.route[this.route.length-1].setPosition(position);
	else
		this.route.splice( this.route.length,0, new OSRM.RouteMarker(OSRM.C.TARGET_LABEL, {draggable:true,icon:OSRM.G.icons['marker-target'],dragicon:OSRM.G.icons['marker-target-drag']}, position));
	document.getElementById('delete-target-marker').style.visibility = "visible";
	return this.route.length-1;
},
setVia: function(id, position) {
	// via nodes only between source and target nodes
	if( this.route.length<2 || id > this.route.length-2 )
		return -1;
	
	this.route.splice(id+1,0, new OSRM.RouteMarker(OSRM.C.VIA_LABEL, {draggable:true,icon:OSRM.G.icons['marker-via'],dragicon:OSRM.G.icons['marker-via-drag']}, position));
	return id+1;
},
removeMarker: function(id) {
	if( id >= this.route.length )
		return;
	
	// also remove vias if source or target are removed
	if( id==0 && this.route[0].label == OSRM.C.SOURCE_LABEL ) {
		this.removeVias();
		document.getElementById('input-source-name').value = "";
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-headline').innerHTML = "";
		document.getElementById('delete-source-marker').style.visibility = "hidden";
	} else if( id == this.route.length-1 && this.route[ this.route.length-1 ].label == OSRM.C.TARGET_LABEL ) {
		this.removeVias();
		id = this.route.length-1;
		document.getElementById('input-target-name').value = "";
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-headline').innerHTML = "";
		document.getElementById('delete-target-marker').style.visibility = "hidden";
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
