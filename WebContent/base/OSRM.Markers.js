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

// OSRM marker management (all route markers should only be set and deleted with these routines!)
// [this holds the vital information of the route]

OSRM.Markers = function() {
	this.route = new Array();
	this.highlight = new OSRM.DragMarker("highlight", {zIndexOffset:-1,draggable:true,icon:OSRM.G.icons['marker-highlight'],dragicon:OSRM.G.icons['marker-highlight-drag']});;
	this.hover = new OSRM.Marker("hover", {zIndexOffset:-1,draggable:false,icon:OSRM.G.icons['marker-highlight']});;
	this.dragger = new OSRM.DragMarker("drag", {draggable:true,icon:OSRM.G.icons['marker-drag'],dragicon:OSRM.G.icons['marker-drag']});;
};
OSRM.extend( OSRM.Markers,{
reset: function() {
	// remove route markers
	for(var i=0; i<this.route.length;i++)
		this.route[i].hide();
	this.route.splice(0, this.route.length);
	document.getElementById('gui-delete-source').style.visibility = "hidden";
	document.getElementById('gui-delete-target').style.visibility = "hidden";
	// remove special markers
	this.highlight.hide();
	this.dragger.hide();
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
	document.getElementById('gui-delete-source').style.visibility = "visible";
	return 0;	
},
setTarget: function(position) {
	// target node is always last node
	if( this.route[this.route.length-1] && this.route[ this.route.length-1 ].label == OSRM.C.TARGET_LABEL )
		this.route[this.route.length-1].setPosition(position);
	else
		this.route.splice( this.route.length,0, new OSRM.RouteMarker(OSRM.C.TARGET_LABEL, {draggable:true,icon:OSRM.G.icons['marker-target'],dragicon:OSRM.G.icons['marker-target-drag']}, position));
	document.getElementById('gui-delete-target').style.visibility = "visible";
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
		document.getElementById('gui-input-source').value = "";
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-header').innerHTML = "";
		document.getElementById('gui-delete-source').style.visibility = "hidden";
	} else if( id == this.route.length-1 && this.route[ this.route.length-1 ].label == OSRM.C.TARGET_LABEL ) {
		this.removeVias();
		id = this.route.length-1;
		document.getElementById('gui-input-target').value = "";
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-header').innerHTML = "";
		document.getElementById('gui-delete-target').style.visibility = "hidden";
	}
	
	this.route[id].hide();
	this.route.splice(id, 1);
},
reverseMarkers: function() {
	var size = this.route.length;
	
	// invert route, if a route is shown	
	if( size > 1 ) {
		// switch positions in nodes
		var temp_position = this.route[0].getPosition();
		this.route[0].setPosition( this.route[size-1].getPosition() );
		this.route[size-1].setPosition( temp_position );
		// switch nodes in array
		var temp_node = this.route[0];
		this.route[0] = this.route[size-1];
		this.route[size-1] = temp_node;
		// reverse route
		this.route.reverse();
		// clear information (both delete markers stay visible)
		document.getElementById('information-box').innerHTML = "";
		document.getElementById('information-box-header').innerHTML = "";
		
	// invert marker, if only one marker is shown (implicit clear of information / delete markers)
	} else if( size > 0 ) {
		var position = this.route[0].getPosition();
		var label = this.route[0].label;
		this.removeMarker(0);
		if( label == OSRM.C.TARGET_LABEL )			
			this.setSource( position );
		else if( label == OSRM.C.SOURCE_LABEL )
			this.setTarget( position );
		this.route[0].show();
	}

},
hasSource: function() {
	if( this.route[0] && this.route[0].label == OSRM.C.SOURCE_LABEL )
		return true;
	return false;
},
hasTarget: function() {
	if( this.route[this.route.length-1] && this.route[this.route.length-1].label == OSRM.C.TARGET_LABEL )
		return true;
	return false;
},

//relabel all via markers
relabelViaMarkers: function() {
	for(var i=1, size=this.route.length-1; i<size; i++)
		this.route[i].marker.setLabel(i);
}
});