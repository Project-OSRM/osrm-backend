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

// Leaflet extension: Dashed Polyline
// [adds dashed optionally dashed lines when using SVG or VML rendering]


// dashed polyline class
L.DashedPolyline = L.Polyline.extend({
	initialize: function(latlngs, options) {
		L.Polyline.prototype.initialize.call(this, latlngs, options);
	},
	
	options: {
		dashed: true
	}
});


// svg rendering
L.DashedPolyline = !L.Browser.svg ? L.DashedPolyline : L.DashedPolyline.extend({
	_updateStyle: function () {
		L.Polyline.prototype._updateStyle.call(this);
		if (this.options.stroke) {
			if (this.options.dashed == true)
				this._path.setAttribute('stroke-dasharray', '8,6');
			else
				this._path.setAttribute('stroke-dasharray', '');
		}
	}
});


// vml rendering
L.DashedPolyline = L.Browser.svg || !L.Browser.vml ? L.DashedPolyline : L.DashedPolyline.extend({
	_updateStyle: function () {
		L.Polyline.prototype._updateStyle.call(this);
		if (this.options.stroke) {
			if (this.options.dashed == true)
				this._stroke.dashstyle = "dash";
			else
				this._stroke.dashstyle = "solid";
		}
	}
	
});
