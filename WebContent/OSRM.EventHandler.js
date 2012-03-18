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

// OSRM EventHandler
// [adds simple event handling: other classes can derive from this class to acquire custom event handling]


OSRM.EventHandler = function() {
	this._listeners = {};
};

OSRM.extend( OSRM.EventHandler, {
	
	// add listener
	addListener: function(type, listener) {
		if( this._listeners[type] == undefined)
			this._listeners[type] = [];
		this._listeners[type].push(listener);
	},
	
	//remove event listener
	removeListener: function(type, listener) {
		if( this._listeners[type] != undefined) {
			for(var i=0; i<this._listeners[type].length; i++)
				if( this._listeners[type][i] == listener) {
					this._listeners[type].splice(i,1);
					break;
				}
		}
	},
	
	// fire event
	fire: function(event) {
		if( typeof event == "string")
			event = {type:event};
		if( !event.target )
			event.target = this;
		
		if( !event.type )
			throw new Error("event object missing type property!");
		
		if( this._listeners[type] != undefined)
			for(var listener in this._listeners[event.type])
				listener.call(this, event);
	}
	
});
