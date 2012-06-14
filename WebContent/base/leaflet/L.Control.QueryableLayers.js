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
// [simply Control.Layers extended by query functions and some fixes for touch screens]
L.Control.QueryableLayers = L.Control.Layers.extend({
	
// changes to leaflet behaviour
_initLayout: function () {
	this._container = L.DomUtil.create('div', 'leaflet-control-layers');
	L.DomEvent.disableClickPropagation(this._container);							// change to Leaflet: always disable click propagation 

	this._form = L.DomUtil.create('form', 'leaflet-control-layers-list');

	if (this.options.collapsed) {
		L.DomEvent.addListener(this._container, 'mouseover', this._expand, this);
		L.DomEvent.addListener(this._container, 'mouseout', this._collapse, this);

		var link = this._layersLink = L.DomUtil.create('a', 'leaflet-control-layers-toggle');
		link.href = '#';
		link.title = 'Layers';

		if (L.Browser.touch) {
			L.DomEvent.addListener(link, 'click', this._expand, this);
			L.DomEvent.disableClickPropagation(link);								// change to Leaflet: disable click propagation
		} else {
			L.DomEvent.addListener(link, 'focus', this._expand, this);
		}
		this._map.on('movestart', this._collapse, this);
		// TODO keyboard accessibility

		this._container.appendChild(link);
	} else {
		this._expand();
	}

	this._baseLayersList = L.DomUtil.create('div', 'leaflet-control-layers-base', this._form);
	this._separator = L.DomUtil.create('div', 'leaflet-control-layers-separator', this._form);
	this._overlaysList = L.DomUtil.create('div', 'leaflet-control-layers-overlays', this._form);

	this._container.appendChild(this._form);
},

// new query functionality
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
}
});
