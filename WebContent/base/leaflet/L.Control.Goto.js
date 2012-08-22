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

// goto Layers control
// [used to add clickable buttons to the map]
L.Control.Goto = L.Control.extend({
	options: {
		position: 'topright'
	},

	onAdd: function (map) {
		var className = 'leaflet-control-goto',
		    container = L.DomUtil.create('div', className);

		this._createButton('Goto Location', className + '-my-location', container, map.zoomIn, map);
		this._createButton('Goto Route', className + '-route', container, map.zoomOut, map);

		return container;
	},

	_createButton: function (title, className, container, fn, context) {
		var link = L.DomUtil.create('a', className, container);
		link.href = '#';
		link.title = title;

		L.DomEvent
			.on(link, 'click', L.DomEvent.stopPropagation)
			.on(link, 'click', L.DomEvent.preventDefault)
			.on(link, 'click', fn, context)
			.on(link, 'dblclick', L.DomEvent.stopPropagation);

		return link;
	}
});

L.Map.mergeOptions({
	gotoControl: true
});

L.Map.addInitHook(function () {
	if (this.options.gotoControl) {
		this.gotoControl = new L.Control.Goto();
		this.addControl(this.gotoControl);
	}
});

L.control.Goto = function (options) {
	return new L.Control.Goto(options);
};