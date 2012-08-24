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
activeExclusive: undefined,
activeTooltip: undefined,
tooltips: {},
	// LOCALIZATION:	deactivation triggered in OSRM.Localization.setLanguageWrapper
	// CLICKING:		deactivation triggered in OSRM.Map.click
	// DRAGGING:		deactivation triggered in OSRM.Routing.getRoute_Dragging
	// MAINTENANCE:		deactivation only by config


// initialize notifications and tooltip timers
init: function() {
	// exclusive notifications
	var config = OSRM.DEFAULTS.NOTIFICATIONS;
	if( config["MAINTENANCE"] == true ) {
		var header = OSRM.DEFAULTS.OVERRIDE_MAINTENANCE_NOTIFICATION_HEADER || OSRM.loc("NOTIFICATION_MAINTENANCE_HEADER");
		var body = OSRM.DEFAULTS.OVERRIDE_MAINTENANCE_NOTIFICATION_BODY || OSRM.loc("NOTIFICATION_MAINTENANCE_BODY");
		OSRM.GUI.exclusiveNotify( header, body, false);
		OSRM.GUI.activeExclusive = "MAINTENANCE";
		return;
	}
	
	// tooltip timers
	var config = OSRM.DEFAULTS.NOTIFICATIONS;	
	var tooltips = OSRM.GUI.tooltips;
	for( id in config ) {
		// skip notification?
		if( !OSRM.Utils.isNumber( config[id] ) )
			continue;		
		// create structure to hold timer data
		tooltips[id] = {};
		// start timer
		tooltips[id]._timer = setTimeout(
				function(_id){ return function(){OSRM.GUI._showTooltip(_id);}; }(id),
				config[id]
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
	// only show tooltip if it exists
	if(tooltips[id] == undefined)
		return;	
	
	// only show tooltip if it is still pending 
	if( tooltips[id]._pending == false ) {
		return;
	}
		
	// if a notification is currently shown, restart timer
	var config = OSRM.DEFAULTS.NOTIFICATIONS;
	if( OSRM.GUI.isTooltipVisible() ) {
		tooltips[id]._timer = setTimeout(
				function(_id){ return function(){OSRM.GUI._showTooltip(_id);}; }(id),
				config[id]
		);		
		return;
	}
	
	// show notification
	OSRM.GUI.tooltipNotify( OSRM.loc("NOTIFICATION_"+id+"_HEADER"), OSRM.loc("NOTIFICATION_"+id+"_BODY") );
	OSRM.GUI.activeTooltip = id;
	
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
	OSRM.GUI.exclusiveResize();
},
exclusiveDenotify: function() {
	document.getElementById('exclusive-notification-blanket').style.display = "none";
	OSRM.GUI.activeExclusive = undefined;
},
exclusiveUpdate: function() {
	if( OSRM.GUI.activeExclusive == undefined )
		return;
	
	// override mainly intended for maintenance mode
	var header = OSRM.DEFAULTS["OVERRIDE_"+OSRM.GUI.activeExclusive+"_HEADER"] || OSRM.loc("NOTIFICATION_MAINTENANCE_HEADER");
	var body = OSRM.DEFAULTS["OVERRIDE_"+OSRM.GUI.activeExclusive+"_BODY"] || OSRM.loc("NOTIFICATION_MAINTENANCE_BODY");
	
	document.getElementById('exclusive-notification-label').innerHTML = header;
	document.getElementById('exclusive-notification-box').innerHTML = body;
	OSRM.GUI.exclusiveResize();
},
exclusiveResize: function() {
	var height = document.getElementById('exclusive-notification-box').clientHeight;
	document.getElementById('exclusive-notification-content').style.height = (height + 28) + "px";
	document.getElementById('exclusive-notification-wrapper').style.height = (height + 48) + "px";
},
inMaintenance: function() {
	return OSRM.GUI.activeExclusive == "MAINTENANCE"; 
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
tooltipDenotify: function() {
	document.getElementById('tooltip-notification-wrapper').style.display = "none";
	OSRM.GUI.activeTooltip = undefined;
},
tooltipUpdate: function() {
	if( OSRM.GUI.activeTooltip == undefined )
		return;
	document.getElementById('tooltip-notification-label').innerHTML = OSRM.loc("NOTIFICATION_"+OSRM.GUI.activeTooltip+"_HEADER");
	document.getElementById('tooltip-notification-box').innerHTML = OSRM.loc("NOTIFICATION_"+OSRM.GUI.activeTooltip+"_BODY");
	OSRM.GUI.tooltipResize();
	OSRM.GUI.tooltipResize();															// simple trick to retain opened/closed state of tooltip
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
isTooltipVisible: function() {
	return document.getElementById('tooltip-notification-wrapper').style.display == "block"; 
},


// update language of any notification
updateNotifications: function() {
	OSRM.GUI.exclusiveUpdate();
	OSRM.GUI.tooltipUpdate();
}

});
