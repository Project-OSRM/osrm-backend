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

// Leaflet extension: LabelMarkerIcon
// [icon class with extra label and simple icon changing]


// extended icon class 
L.LabelMarkerIcon = L.Icon.extend({
	// altered icon creation (with label)
	_createImg: function (src) {
		var el;
		if (!L.Browser.ie6) {
			el = document.createElement('div');
			
			var img = document.createElement('img');
			var num = document.createElement('div');
			img.src = src;
			num.className = 'via-counter';
			num.innerHTML = "";
			
			el.appendChild(img);
			el.appendChild(num);
		} else {
			el = document.createElement('div');
			el.style.filter = 'progid:DXImageTransform.Microsoft.AlphaImageLoader(src="' + src + '")';
		}
		return el;
	},

	// non-destructive icon changing
	changeIcon: function (el) {
		return this._changeIcon('icon', el);
	},

	changeShadow: function (el) {
		return this.options.shadowUrl ? this._changeIcon('shadow', el) : null;
	},
	
	_changeIcon: function (name, el) {
		var src = this._getIconUrl(name);
		if (!src) {
			if (name === 'icon') {
				throw new Error("iconUrl not set in Icon options (see the docs).");
			}
			return null;
		}		
		
		var img = this._changeImg(src, el);
		this._setIconStyles(img, name);
		
		return img;
	},	
	
	_changeImg: function (src, el) {
		if (!L.Browser.ie6) {
			el.firstChild.src = src;
		} else {
			el.style.filter = 'progid:DXImageTransform.Microsoft.AlphaImageLoader(src="' + src + '")';
		}
		return el;
	}
});
