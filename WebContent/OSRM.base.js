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

// OSRM base class
// [has to loaded before all other OSRM classes]

OSRM = {};
OSRM.VERSION = '0.1.1';


// inheritance helper function (convenience function)
OSRM._inheritFromHelper = function() {};
OSRM.inheritFrom = function( sub_class, base_class ) {
	OSRM._inheritFromHelper.prototype = base_class.prototype;
	sub_class.prototype = new OSRM._inheritFromHelper();
	sub_class.prototype.constructor = sub_class;
	sub_class.prototype.base = base_class.prototype;
};


// class prototype extending helper function (convenience function)
OSRM.extend = function( target_class, properties ) {
	for( property in properties ) {
		target_class.prototype[property] = properties[property];
	}
};


// usage:
// SubClass = function() {
// 	SubClass.prototype.base.constructor.apply(this, arguments);
// }
// OSRM.inheritFrom( SubClass, BaseClass );
// OSRM.extend( SubClass, { property:value } );
