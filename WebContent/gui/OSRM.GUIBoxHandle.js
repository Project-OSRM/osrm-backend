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

// OSRM GUIBoxHandle
// [performs showing and hiding of UI boxes]

OSRM.GUIBoxHandle = function( box_name, side, css, transitionStartFct, transitionEndFct  ) {
	// do not create handle if box does not contain a toggle button
	var toggle = document.getElementById( box_name + '-toggle');
	if( toggle == null ) {
		console.log("[error] No toggle button for " + box_name);
		return;
	}
	
	// create handle DOM elements
	var wrapper = document.createElement('div');
	wrapper.id = box_name + '-handle-wrapper';
	wrapper.className = 'box-wrapper box-handle-wrapper-'+side;
	wrapper.style.cssText += css;
	var content = document.createElement('div');
	content.id = box_name + '-handle-content';
	content.className = 'box-content box-handle-content-'+side;
	var icon = document.createElement('div');
	icon.id = box_name + '-handle-icon';
	icon.className = 'iconic-button';
	icon.title = box_name;
	
	content.appendChild(icon);
	wrapper.appendChild(content);
	document.body.appendChild(wrapper);
	
	// create attributes
	this._box = document.getElementById( box_name + '-wrapper' );
	this._class = this._box.className;
	this._width = this._box.clientWidth;
	this._side = side;
	this._handle = wrapper;
	this._box_group = null;
	this._transitionEndFct = transitionEndFct;

	// hide box and show handle by default
	this._box.style[this._side]=-this._width+"px";
	this._box_visible = false;
	this._box.style.visibility="hidden";
	this._handle.style.visibility="visible";

	// add functionality
	var full_fct = transitionStartFct ? OSRM.concat(this._toggle, transitionStartFct) : this._toggle;
	var fct = OSRM.bind( this, full_fct );
	toggle.onclick = fct;
	icon.onclick = fct;
	
	var full_fct = transitionEndFct ? OSRM.concat(this._onTransitionEnd, transitionEndFct) : this._onTransitionEnd;
	var fct = OSRM.bind( this, full_fct );	
	if( OSRM.Browser.FF3==-1 && OSRM.Browser.IE6_9==-1 ) {
		var box_wrapper = document.getElementById(box_name + '-wrapper');
		box_wrapper.addEventListener("transitionend", fct, false);
		box_wrapper.addEventListener("webkitTransitionEnd", fct, false);
		box_wrapper.addEventListener("oTransitionEnd", fct, false);
		box_wrapper.addEventListener("MSTransitionEnd", fct, false);
	} else {
		this._legacyTransitionEndFct = fct;			// legacy browser support
	}
};

OSRM.extend( OSRM.GUIBoxHandle, {
boxVisible: function() {
	return this._box_visible;
},
boxWidth: function() {
	return this._width;
},

$addToGroup: function(group) {
	this._box_group = group;
},
$show: function() {
	this._handle.style.visibility="visible";
},
$hide: function() {
	this._handle.style.visibility="hidden";
},
$showBox: function() {
	this._box_visible = true;
	this._box.style.visibility="visible";
	this._handle.style.visibility="hidden";
	this._box.style[this._side]="5px";
},
$hideBox: function() {
	this._box_visible = false;
	this._box.style.visibility="hidden";
	this._handle.style.visibility="visible";
	this._box.style[this._side]=-this._width+"px";
},

_toggle: function() {
	this._box.className += " box-animated";
	if( this._box_visible == false ) {
		this._box_group.$hide();
		this._box.style[this._side]="5px";
		this._box.style.visibility="visible";			// already show box, so that animation is seen
	} else {
		this._box.style[this._side]=-this._width+"px";
	}
	// legacy browser support
	if( OSRM.Browser.FF3!=-1 || OSRM.Browser.IE6_9!=-1 )
		setTimeout(this._legacyTransitionEndFct, 0);
},
_onTransitionEnd: function() {
	this._box.className = this._class;
	if( this._box_visible == true ) {
		this._box_group.$show();
		this._box_visible = false;
		this._box.style.visibility="hidden";
	} else {
		this._box_visible = true;
		this._box.style.visibility="visible";
	}	
}
});