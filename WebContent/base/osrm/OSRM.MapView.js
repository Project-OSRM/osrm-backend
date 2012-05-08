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

// map view/model
// [extending Leaflet L.Map with setView/fitBounds methods that respect UI visibility, better layerControl] 
OSRM.MapView = L.Map.extend({
	setViewUI: function(position, zoom, no_animation) {
		if( OSRM.G.main_handle.boxVisible() ) {
			var point = this.project( position, zoom);
			point.x-=OSRM.G.main_handle.boxWidth()/2;
			position = this.unproject(point,zoom);		
		}
		this.setView( position, zoom, no_animation);	
	},
	fitBoundsUI: function(bounds) {
		var southwest = bounds.getSouthWest();
		var northeast = bounds.getNorthEast();
		var zoom = this.getBoundsZoom(bounds);
		var sw_point = this.project( southwest, zoom);
		if( OSRM.G.main_handle.boxVisible() )
			sw_point.x-=OSRM.G.main_handle.boxWidth()+20;
		else
			sw_point.x-=20;
		sw_point.y+=20;
		var ne_point = this.project( northeast, zoom);
		ne_point.y-=20;
		ne_point.x+=20;
		bounds.extend( this.unproject(sw_point,zoom) );
		bounds.extend( this.unproject(ne_point,zoom) );
		this.fitBounds( bounds );	
	},
	getBoundsUI: function(unbounded) {
		var bounds = this.getPixelBounds();
		if( OSRM.G.main_handle.boxVisible() )
			bounds.min.x+=OSRM.G.main_handle.boxWidth();
		var sw = this.unproject(new L.Point(bounds.min.x, bounds.max.y), this._zoom, true),
			ne = this.unproject(new L.Point(bounds.max.x, bounds.min.y), this._zoom, true);
		return new L.LatLngBounds(sw, ne);		
	},	
	getCenterUI: function(unbounded) {
		var viewHalf = this.getSize();
		if( OSRM.G.main_handle.boxVisible() )
			viewHalf.x += OSRM.G.main_handle.boxWidth();
		var centerPoint = this._getTopLeftPoint().add(viewHalf.divideBy(2));
		
		return this.unproject(centerPoint, this._zoom, unbounded);
	},
	addLayerControl: function( layerControl ) {
		if( this.layerControl )
			return;
		
		this.layerControl = layerControl;
		this.addControl(this.layerControl);
	},
	getActiveLayerId: function() {
		var tile_server_id = 0;
		
		var tile_servers = OSRM.DEFAULTS.TILE_SERVERS;
		var tile_server_name = this.layerControl.getActiveLayerName();
		for(var i=0, size=tile_servers.length; i<size; i++) {
			if( tile_servers[i].display_name == tile_server_name ) {
				tile_server_id = i;
				break;
			}
		}
		
		return tile_server_id;
	}
});
