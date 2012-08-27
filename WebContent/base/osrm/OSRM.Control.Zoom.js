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

// zoom control
// [modified zoom control with ids, prevention of click propagation, show/hide with respect to main OSRM window]
OSRM.Control = OSRM.Control || {};
OSRM.Control.Zoom = L.Control.extend({
	options: {
		position: 'topleft'
	},

	onAdd: function (map) {
		// unique control
		if( document.getElementById('gui-control-zoom') )
			return document.getElementById('gui-control-zoom');
		
		// create wrapper
		var container = L.DomUtil.create('div', 'box-wrapper gui-control-wrapper');
		container.id = 'gui-control-zoom';
		L.DomEvent.disableClickPropagation(container);
		
		// create buttons
		this._createButton('gui-zoom-in', container, map.zoomIn, map, true);
		this._createButton('gui-zoom-out', container, map.zoomOut, map, true);

		return container;
	},

	_createButton: function (id, container, fn, context, isActive) {
		var inactive = (isActive == false) ? "-inactive" : "";
		var classNames = "box-content" + " " + "gui-control"+inactive + " " + id+inactive;		
		var link = L.DomUtil.create('a', classNames, container);
		link.id = id;
		link.title = id;

		L.DomEvent
			.on(link, 'click', L.DomEvent.stopPropagation)
			.on(link, 'click', L.DomEvent.preventDefault)
			.on(link, 'click', fn, context)
			.on(link, 'dblclick', L.DomEvent.stopPropagation);

		return link;
	},
	
	hide: function() {
		var zoom_controls = document.getElementById("gui-control-zoom");
		if( zoom_controls )
			zoom_controls.style.visibility="hidden";		
	},
	
	show: function() {
		var zoom_controls = document.getElementById("gui-control-zoom");
		if( zoom_controls ) {
			zoom_controls.style.top = "5px";
			zoom_controls.style.left = ( OSRM.G.main_handle.boxVisible() == true ? (OSRM.G.main_handle.boxWidth()+10) : "30") + "px";
			zoom_controls.style.visibility="visible";
		}		
	}
});
