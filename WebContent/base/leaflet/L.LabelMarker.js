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

// Leaflet extension: LabelMarker
// [marker class that allows for changing icons while dragging]


// extended marker class
L.LabelMarker = L.Marker.extend({
	// change marker icon
	changeIcon: function( icon ) {
		this.options.icon = icon;

		if (this._map) {
			this._changeIcon();
		}
	},

	// add/change marker label
	setLabel: function( label ) {
		if(this._icon) {
			this._icon.lastChild.innerHTML=label;
			this._icon.lastChild.style.display = "block";
		}
	},
	
	// add/change marker tooltip
	setTitle: function ( title ) {
		this.options.title = title;
		this._icon.title = title;
	},
	
	// actual icon changing routine
	_changeIcon: function () {
		var options = this.options,
	    	map = this._map,
	    	animation = (map.options.zoomAnimation && map.options.markerZoomAnimation),
	    	classToAdd = animation ? 'leaflet-zoom-animated' : 'leaflet-zoom-hide';

		if (this._icon) {
			this._icon = options.icon.changeIcon( this._icon );
			L.DomUtil.addClass(this._icon, classToAdd);
			L.DomUtil.addClass(this._icon, 'leaflet-clickable');
		}
	}
});