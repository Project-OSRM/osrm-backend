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

// Leaflet extension: MouseMarker
// [marker class that propagates modifier and button presses in mouse click events and allows for changing icons]


// extended marker class
L.MouseMarker = L.Marker.extend({
	initialize: function (latlng, options) {
		L.Marker.prototype.initialize.apply(this, arguments);
	},
	
	switchIcon: function( icon ) {
		this.options.icon = icon;

		if (this._map) {
			this._changeIcon();
			this._reset();
		}
	},
	
	_changeIcon: function () {
		var options = this.options;

		if (this._icon) {
			this._icon = options.icon.switchIcon( this._icon );
			this._icon.title = options.title;
		}
		
		var panes = this._map._panes;
		
		if (this._shadow)
			panes.shadowPane.removeChild(this._shadow);
		this._shadow = options.icon.createShadow();
		if (this._shadow)
			panes.shadowPane.appendChild(this._shadow);
	},	

	_onMouseClick: function (e) {
		L.DomEvent.stopPropagation(e);
		if (this.dragging && this.dragging.moved()) { return; }
		this.fire(e.type, {
			altKey: e.altKey,
			ctrlKey: e.ctrlKey,
			shiftKey: e.shiftKey,
			button: e.button
		});
	}
});