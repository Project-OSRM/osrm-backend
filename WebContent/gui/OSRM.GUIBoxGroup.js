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

// OSRM MainGUI
// [handles all GUI events that interact with appearance of main window]


// OSRM GUIBoxGroup
// [group UI boxes so that handles can be shown/hidden together]

OSRM.GUIBoxGroup = function() {
	this._handles = [];
};

OSRM.extend( OSRM.GUIBoxGroup, {
add: function( handle ) {
	this._handles.push( handle );
	handle.$addToGroup(this);
},
select: function( handle ) {
	for(var i=0; i< this._handles.length; i++) {
		if( this._handles[i] != handle )
			this._handles[i].$hideBox();
		else
			this._handles[i].$showBox();
	}
},

$hide: function() {
	for(var i=0; i< this._handles.length; i++) {
		this._handles[i].$hide();
	}
},
$show: function() {
	for(var i=0; i< this._handles.length; i++) {
		this._handles[i].$show();
	}
}
});