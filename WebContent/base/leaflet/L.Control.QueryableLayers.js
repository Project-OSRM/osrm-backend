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

// queryable Layers control
// [extension of Layers.Control with OSRM styling and additional query methods]
L.Control.QueryableLayers = L.Control.Layers.extend({
	
// query functionality
getActiveLayerName: function () {
	var i, input, obj,
	inputs = this._form.getElementsByTagName('input'),
	inputsLen = inputs.length;

	for (i = 0; i < inputsLen; i++) {
		input = inputs[i];
		obj = this._layers[input.layerId];
		if (input.checked && !obj.overlay) {
			return obj.name;
		}
	}
},
getActiveLayer: function () {
	var i, input, obj,
	inputs = this._form.getElementsByTagName('input'),
	inputsLen = inputs.length;

	for (i = 0; i < inputsLen; i++) {
		input = inputs[i];
		obj = this._layers[input.layerId];
		if (input.checked && !obj.overlay) {
			return obj.layer;
		}
	}
},


// overwrite Control.Layers methods to get OSRM styling 
_expand: function () {
	L.DomUtil.addClass(this._container, 'leaflet-control-layers-expanded');
},
_collapse: function () {
	this._container.className = this._container.className.replace(' leaflet-control-layers-expanded', '');
}

});
