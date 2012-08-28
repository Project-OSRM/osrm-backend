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

// locations control
// [navigation buttons for important locations - zoom on route, zoom on user]
OSRM.Control.Locations = L.Control.extend({
	options: {
		position: 'topright'
	},
	
	onAdd: function (map) {
		// create wrapper
		var container = L.DomUtil.create('div', 'box-wrapper gui-control-wrapper');
		L.DomEvent.disableClickPropagation(container);
		
		// create buttons
		this._userButton = this._createButton('gui-locations-user', container, OSRM.GUI.zoomOnUser, map, !!navigator.geolocation );
		this._routeButton = this._createButton('gui-locations-route', container, OSRM.GUI.zoomOnRoute, map, false);

		this._container = container;		
		return container;
	},

	_createButton: function (id, container, fn, context, isActive) {
		var inactive = (isActive == false) ? "-inactive" : "";
		var classNames = "box-content" + " " + "gui-control"+inactive + " " + id+inactive;
		var link = L.DomUtil.create('a', classNames, container);
		link.title = id;

		L.DomEvent
			.on(link, 'click', L.DomEvent.stopPropagation)
			.on(link, 'click', L.DomEvent.preventDefault)
			.on(link, 'click', fn, context)
			.on(link, 'dblclick', L.DomEvent.stopPropagation);

		return link;
	},
	
	activateRoute: function() {
		this._routeButton.className = "box-content gui-control gui-locations-route";		
	},
	deactivateRoute: function() {
		this._routeButton.className = "box-content gui-control-inactive gui-locations-route-inactive";		
	},
	setTooltips: function( userButton, routeButton) {
		this._userButton.title = userButton;
		this._routeButton.title = routeButton;
	}
});
