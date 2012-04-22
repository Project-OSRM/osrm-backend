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

// Leaflet extension: SwitchableIcon
// [will be an extension of L.Icon in Leaflet 0.4, for now it is a copy with added functionality]


// icon class with functions to simply switch the icon images
L.SwitchableIcon = L.Class.extend({
	options: {
		/*
		iconUrl: (String) (required)
		iconSize: (Point) (can be set through CSS)
		iconAnchor: (Point) (centered by default if size is specified, can be set in CSS with negative margins)
		popupAnchor: (Point) (if not specified, popup opens in the anchor point)
		shadowUrl: (Point) (no shadow by default)
		shadowSize: (Point)
		*/
		className: ''
	},

	initialize: function (options) {
		L.Util.setOptions(this, options);
	},

	createIcon: function () {
		return this._createIcon('icon');
	},

	createShadow: function () {
		return this.options.shadowUrl ? this._createIcon('shadow') : null;
	},

	_createIcon: function (name) {
		var img = this._createImg(this.options[name + 'Url']);
		this._setIconStyles(img, name);
		return img;
	},

	_setIconStyles: function (img, name) {
		var options = this.options,
			size = options[name + 'Size'],
			anchor = options.iconAnchor;

		if (!anchor && size) {
			anchor = size.divideBy(2, true);
		}

		if (name === 'shadow' && anchor && options.shadowOffset) {
			anchor._add(options.shadowOffset);
		}

		img.className = 'leaflet-marker-' + name + ' ' + options.className;

		if (anchor) {
			img.style.marginLeft = (-anchor.x) + 'px';
			img.style.marginTop  = (-anchor.y) + 'px';
		}

		if (size) {
			img.style.width  = size.x + 'px';
			img.style.height = size.y + 'px';
		}
	},

	_createImg: function (src) {
		var el;
		if (!L.Browser.ie6) {
			el = document.createElement('img');
			el.src = src;
		} else {
			el = document.createElement('div');
			el.style.filter = 'progid:DXImageTransform.Microsoft.AlphaImageLoader(src="' + src + '")';
		}
		return el;
	},

	// new functions start here
	switchIcon: function (el) {
		return this._switchIcon('icon', el);
	},

	switchShadow: function (el) {
		return this.options.shadowUrl ? this._switchIcon('shadow', el) : null;
	},

	_switchIcon: function (name, el) {
		var img = this._switchImg(this.options[name + 'Url'], el);
		this._setIconStyles(img, name);
		return img;
	},	
	
	_switchImg: function (src, el) {
		if (!L.Browser.ie6) {
			el.src = src;
		} else {
			el.style.filter = 'progid:DXImageTransform.Microsoft.AlphaImageLoader(src="' + src + '")';
		}
		return el;
	}	
});
