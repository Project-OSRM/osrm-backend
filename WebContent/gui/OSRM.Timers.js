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

// OSRM Timers
// [handles notification timers]


OSRM.GUI.extend( {
	
// notifications
notifications: [
	{	time:	30000,
		header: "[Tooltip] Clicking and Dragging",
		body: 	"You can simply click on the map to set source and target markers. " +
				"Then you can continue and drag the markers over the map or create. " +
				"<br/><br/>" +
				"You can even create additional markers by dragging them off of the main route." + 
				"Markers can be simply deleted by clicking on them.",
		_class:	"Routing",
		_func:	"getRoute_Dragging"
	}
],
		
// initialize notification timers
init: function() {
	// init variables
	var notifications = OSRM.GUI.notifications;	
	OSRM.G.notification_timers = new Array( notifications.length );

	// init timers
	for( var i=0, iEnd=notifications.length; i<iEnd; ++i) {
		notifications[i].timer = setTimeout( function(id){ return function(){ OSRM.GUI.notification_timeout(id);}; }(i), notifications[i].time);

		notifications[i].old_function = OSRM[notifications[i]._class][notifications[i]._func];
		OSRM[notifications[i]._class][notifications[i]._func] = function(id){ return function(){ OSRM.GUI.notification_wrapper(id);}; }(i);
	}
},

// wrapper function to clear timeouts
notification_wrapper: function(id) {
	var notifications = OSRM.GUI.notifications;
	
	clearTimeout( notifications[id].timer );
	notifications[id].old_function();
	OSRM[notifications[id]._class][notifications[id]._func] = notifications[id].old_function;	
},

// show notification message after timeout expired
notification_timeout: function(id) {
	OSRM.notify( OSRM.GUI.notifications[id].header, OSRM.GUI.notifications[id].body, true );
},

// clear notification timeout
notification_clear: function(id) {
	clearTimeout( OSRM.GUI.notifications[id].timer );
}

});
