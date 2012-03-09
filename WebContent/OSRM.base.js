// OSRM base class

OSRM = {};
OSRM.VERSION = '0.1';

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
