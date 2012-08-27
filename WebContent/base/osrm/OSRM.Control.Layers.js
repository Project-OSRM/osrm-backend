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

// OSRM Layers control
// [extension of Layers.Control with OSRM styling and additional query methods]
OSRM.Control.Layers = L.Control.Layers.extend({
	
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
onAdd: function (map) {
	this._initLayout(map);
	this._update();

	return this._container;
},
_initLayout: function (map) {
	var className = 'leaflet-control-layers',
	    container = this._container = L.DomUtil.create('div', className);

	if (!L.Browser.touch) {
		L.DomEvent.disableClickPropagation(container);
	} else {
		L.DomEvent.on(container, 'click', L.DomEvent.stopPropagation);
	}

	var form = this._form = L.DomUtil.create('form', className + '-list');

	if (this.options.collapsed) {
		L.DomEvent
			.on(container, 'mouseover', this._expand, this)
			.on(container, 'mouseout', this._collapse, this);

		var link = this._layersLink = L.DomUtil.create('a', className + '-toggle', container);
		link.href = '#';
		link.title = 'Layers';

		if (L.Browser.touch) {
			L.DomEvent
				.on(link, 'click', L.DomEvent.stopPropagation)
				.on(link, 'click', L.DomEvent.preventDefault)
				.on(link, 'click', this._expand, this);
		}
		else {
			L.DomEvent.on(link, 'focus', this._expand, this);
		}

		this._map.on('movestart', this._collapse, this);
		// TODO keyboard accessibility
	} else {
		this._expand();
	}

	this._baseLayersList = L.DomUtil.create('div', className + '-base', form);
	this._separator = L.DomUtil.create('div', className + '-separator', form);
	this._overlaysList = L.DomUtil.create('div', className + '-overlays', form);

	container.appendChild(form);
},
_expand: function () {
	L.DomUtil.addClass(this._container, 'leaflet-control-layers-expanded');
},
_collapse: function () {
	this._container.className = this._container.className.replace(' leaflet-control-layers-expanded', '');
}

});
