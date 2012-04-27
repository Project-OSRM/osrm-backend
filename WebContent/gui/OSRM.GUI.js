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

// OSRM GUI
// [base GUI class, all other GUI modules extend this class]


OSRM.GUI = {
		
// initialization functions of all GUI parts
init_functions: [],

// init GUI
init: function() {
	for(var i=0, size=OSRM.GUI.init_functions.length; i<size; i++) {
		OSRM.GUI.init_functions[i]();
	}
},

//extend GUI class and add init functions to the array
extend: function( properties ) {
	for( property in properties ) {
		if( property == 'init' )
			OSRM.GUI.init_functions.push( properties[property] );
		else
			OSRM.GUI[property] = properties[property];
	}
} 

};