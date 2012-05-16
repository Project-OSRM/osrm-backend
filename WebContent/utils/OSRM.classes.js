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

// OSRM classes
// [support for inheritance and other function related functionality]

// declare one class to be a subclass of another class
// (runs anonymous function to prevent local functions cluttering global namespace)
(function() {
var _inheritFromHelper = function() {};
OSRM.inheritFrom = function( sub_class, base_class ) {
	_inheritFromHelper.prototype = base_class.prototype;
	sub_class.prototype = new _inheritFromHelper();
	sub_class.prototype.constructor = sub_class;
	sub_class.prototype.base = base_class.prototype;
};
}());


// extend prototypes of a class -> used to add member values and functions
OSRM.extend = function( target_class, properties ) {
	for( property in properties ) {
		target_class.prototype[property] = properties[property];
	}
};


// bind a function to an execution context, i.e. an object (needed for correcting this pointers)
OSRM.bind = function( context, fct1 ) {
	return function() {
		fct1.apply(context, arguments);
	};
};


// concatenate the execution of two functions with the same set of parameters
OSRM.concat = function( fct1, fct2 ) {
	return function() { 
		fct1.apply(this,arguments); 
		fct2.apply(this,arguments); 
	};
};


// [usage of convenience functions]
// SubClass = function() {
// 	SubClass.prototype.base.constructor.apply(this, arguments);
// }
// OSRM.inheritFrom( SubClass, BaseClass );
// OSRM.extend( SubClass, { property:value } );
