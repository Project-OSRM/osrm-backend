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
	{	time:		7000,
		header: 	"[Tooltip] Clicking and Dragging",
		body: 		"You can simply click on the map to set source and target markers. " +
					"Then you can continue and drag the markers over the map or create. " +
					"<br/><br/>" +
					"You can even create additional markers by dragging them off of the main route." + 
					"Markers can be simply deleted by clicking on them.",
		_classes:	["Routing"],
		_funcs:		["getRoute_Dragging"]
	},
	{	time:		4000,
		header: 	"[Tooltip] Clicking and Dragging 2",
		body: 		"You can simply click on the map to set source and target markers. " +
					"Then you can continue and drag the markers over the map or create. " +
					"<br/><br/>" +
					"You can even create additional markers by dragging them off of the main route." + 
					"Markers can be simply deleted by clicking on them.",
		_classes:	["Routing"],
		_funcs:		["getRoute_Dragging"]
	}	
],
		
// initialize notification timers
init: function() {
	// init timers
	var notifications = OSRM.GUI.notifications;	
	OSRM.G.notification_timers = new Array( notifications.length );
	for( var i=0, iEnd=notifications.length; i<iEnd; ++i) {
		// start timer
		notifications[i].timer = setTimeout( function(id){ return function(){ OSRM.GUI.notification_timeout(id);}; }(i), notifications[i].time);

		// create wrapper functions for function calls that will stop the timer
		notifications[i].old_functions = [];
		for(var j=0, jEnd=notifications[i]._classes.length; j<jEnd;j++) {
			notifications[i].old_functions[j] = OSRM[notifications[i]._classes[j]][notifications[i]._funcs[j]];
			OSRM[notifications[i]._classes[j]][notifications[i]._funcs[j]] = function(id,id2){ return function(){ OSRM.GUI.notification_wrapper(id,id2);}; }(i,j);
		}
	}
},

// wrapper function to clear timeouts
notification_wrapper: function(id, id2) {
	var notifications = OSRM.GUI.notifications;
	
	clearTimeout( notifications[id].timer );
	notifications[id].old_functions[id2]();
	for(var j=0, jEnd=notifications[id]._classes.length; j<jEnd;j++) {
		OSRM[notifications[id]._classes[j]][notifications[id]._funcs[j]] = notifications[id].old_functions[j];
	}
},

// show notification message after timeout expired
notification_timeout: function(id) {
	// if a notification is already shown, restart timer
	if( OSRM.isNotifyVisible() ) {
		OSRM.GUI.notifications[id].timer = setTimeout( function(id){ return function(){ OSRM.GUI.notification_timeout(id);}; }(id), OSRM.GUI.notifications[id].time);
		return;
	}
	// show notification
	OSRM.notify( OSRM.GUI.notifications[id].header, OSRM.GUI.notifications[id].body, true );
},

// clear notification timeout
notification_clear: function(id) {
	clearTimeout( OSRM.GUI.notifications[id].timer );
}

});
