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
	{	time:30000,
		header: "[Tooltip] Clicking and Dragging",
		body: 	"You can simply click on the map to set source and target markers. " +
				"Then you can continue and drag the markers over the map or create. " +
				"<br/><br/>" +
				"You can even create additional markers by dragging them off of the main route." + 
				"Markers can be simply deleted by clicking on them."
	}
],
		
// init
init: function() {
	// init variables
	var notifications = OSRM.GUI.notifications;	
	OSRM.G.notification_timers = new Array( notifications.length );

	// init timers
	for( var i=0, iEnd=notifications.length; i<iEnd; ++i)
		OSRM.G.notification_timers[i] = setTimeout( function(id){ return function(){ OSRM.GUI.timeout(id);}; }(i), notifications[i].time);
},

//
timeout: function(id) {
	OSRM.notify( OSRM.GUI.notifications[id].header, OSRM.GUI.notifications[id].body, true );
},

clear_timeout: function(id) {
	clearTimeout( OSRM.G.notification_timers[id] );	
}

});
