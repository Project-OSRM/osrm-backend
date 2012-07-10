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

// OSRM Notifications
// [handles notifications: timed tooltips and exclusive notifications]


OSRM.GUI.extend( {
	
// tooltips
tooltips: {
	// triggered in OSRM.Localization.setLanguageWrapper
	localization:
	{	timeout:	1800000,		// 30min
		header: 	"Did you know? You can change the display language.",
		body: 		"You can use the pulldown menu in the upper left corner to select your favorite language. " +
					"<br/><br/>" +
					"Don't despair if you cannot find your language of choice. " +
					"If you want, you can help to provide additional translations! " +
					"Visit <a href='https://github.com/DennisSchiefer/Project-OSRM-Web'>here</a> for more information."
	},
	// triggered in OSRM.Map.click
	clicking:
	{	timeout:	60000,		// 1min
		header: 	"Did you know? You can click on the map to set route markers.",
		body:		"You can click on the map with the left mouse button to set a source marker (green) or a target marker (red), " +
					"if the source marker already exists. " +
					"The address of the selected location will be displayed in the boxes to the left. " + 
					"<br/><br/>" +
					"You can delete a marker by clicking on it again with the left mouse button."
	},
	// triggered in OSRM.Routing.getRoute_Dragging	
	dragging:
	{	timeout:	120000,		// 2min
		header: 	"Did you know? You can drag each route marker on the map.",
		body:		"You can drag a marker by clicking on it with the left mouse button and holding the button pressed. " +
					"Then you can move the marker around the map and the route will be updated instantaneously. " +
					"<br/><br/>" +
					"You can even create intermediate markers by dragging them off of the main route! "
	}
},


// initialize notifications and tooltip timers
init: function() {
	// notifications
	// [nothing to be done at the moment]
	
	// tooltip timers
	var tooltips = OSRM.GUI.tooltips;
	for( id in tooltips ) {
		// start timer
		tooltips[id]._timer = setTimeout(
				function(_id){ return function(){OSRM.GUI._showTooltip(_id);}; }(id),
				tooltips[id].timeout
		);
		
		// mark tooltip as pending
		tooltips[id]._pending = true;
	}
},


// deactivate pending tooltip
deactivateTooltip: function(id) {
	var tooltips = OSRM.GUI.tooltips;
	if(tooltips[id] == undefined)
		return;	
	
	// mark tooltip as no longer pending
	tooltips[id]._pending = false;
},
// show tooltip after timer expired
_showTooltip: function(id) {
	var tooltips = OSRM.GUI.tooltips;
	if(tooltips[id] == undefined)
		return;	
	
	// only show tooltip if it is still pending 
	if( tooltips[id]._pending == false ) {
		return;
	}
		
	// if a notification is currently shown, restart timer
	if( OSRM.GUI.isTooltipVisible() ) {
		tooltips[id]._timer = setTimeout(
				function(_id){ return function(){OSRM.GUI._showTooltip(_id);}; }(id),
				tooltips[id].timeout
		);		
		return;
	}
	
	// show notification
	OSRM.GUI.tooltipNotify( tooltips[id].header, tooltips[id].body );
	
	// mark tooltip as no longer pending
	tooltips[id]._pending = false;
},


// exclusive notification
exclusiveNotify: function( header, text, closable ){
	document.getElementById('exclusive-notification-blanket').style.display = "block";
	document.getElementById('exclusive-notification-label').innerHTML = header;
	document.getElementById('exclusive-notification-box').innerHTML = text;
	if( closable )
		document.getElementById('exclusive-notification-toggle').onclick = OSRM.GUI.exclusiveDenotify;
	else
		document.getElementById('exclusive-notification-toggle').style.display = "none";
},
exclusiveDenotify: function() {
	document.getElementById('exclusive-notification-blanket').style.display = "none";
},


// tooltip notification
tooltipNotify: function( header, text ){
	document.getElementById('tooltip-notification-wrapper').style.display = "block";
	document.getElementById('tooltip-notification-label').innerHTML = header;
	document.getElementById('tooltip-notification-box').innerHTML = text;
	document.getElementById('tooltip-notification-box').style.display = "block";		// simple trick to always start with a minimized tooltip
	OSRM.GUI.tooltipResize();
	
	document.getElementById('tooltip-notification-toggle').onclick = OSRM.GUI.tooltipDenotify;
	document.getElementById('tooltip-notification-resize').onclick = OSRM.GUI.tooltipResize;
},
tooltipResize: function() {
	if( document.getElementById('tooltip-notification-box').style.display == "none" ) {	
		document.getElementById('tooltip-notification-box').style.display = "block";
		var height = document.getElementById('tooltip-notification-box').clientHeight;
		document.getElementById('tooltip-notification-content').style.height = (height + 28) + "px";
		document.getElementById('tooltip-notification-wrapper').style.height = (height + 48) + "px";
		document.getElementById('tooltip-notification-resize').className = "iconic-button up-marker top-right-button";
	} else {
		document.getElementById('tooltip-notification-box').style.display = "none";
		document.getElementById('tooltip-notification-content').style.height = "18px";
		document.getElementById('tooltip-notification-wrapper').style.height = "38px";		
		document.getElementById('tooltip-notification-resize').className = "iconic-button down-marker top-right-button";
	}
},
tooltipDenotify: function() {
	document.getElementById('tooltip-notification-wrapper').style.display = "none";
},
isTooltipVisible: function() {
	return document.getElementById('tooltip-notification-wrapper').style.display == "block"; 
}

});
