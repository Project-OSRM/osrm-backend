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

// OSRM old/cross browser support
// [browser detection and routines for old/cross browser support] 


// browser detection (runs anonymous function to prevent local variables cluttering global namespace)
(function() {
	var useragent = navigator.userAgent;
	
	OSRM.Browser = {
 		FF3:	useragent.search(/Firefox\/3/),
 		IE6_8:	useragent.search(/MSIE (6|7|8)/),
		IE6_9:	useragent.search(/MSIE (6|7|8|9)/)
	};
}());


// compatibility tools

//add document.head reference for older browsers
document.head = document.head || document.getElementsByTagName('head')[0];

// supply getElementsByClassName method for older browser
OSRM.Browser.getElementsByClassName = function( node, classname ) {
    var a = [];
    var re = new RegExp('(^| )'+classname+'( |$)');
    var els = node.getElementsByTagName("*");
    for(var i=0,j=els.length; i<j; i++)
        if(re.test(els[i].className))a.push(els[i]);
    return a;
};

// call a function when DOM has finished loading and remove event handler (optionally pass a different window object)
OSRM.Browser.onLoadHandler = function( function_pointer, the_window ) {
	the_window = the_window || window;			// default document
	var the_document = the_window.document;
	
	if(the_window.addEventListener) {			// FF, CH, IE9+
		var temp_function = function() { 
			the_window.removeEventListener("DOMContentLoaded", arguments.callee, false);
			function_pointer.call();
		};
		the_window.addEventListener("DOMContentLoaded", temp_function, false);
	}

	else if(the_document.attachEvent) {			// IE8-
		var temp_function = function() { 
			if ( the_document.readyState === "interactive" || the_document.readyState === "complete" ) { 
				the_document.detachEvent("onreadystatechange", arguments.callee); 
				function_pointer.call(); 
			}
		};
		the_document.attachEvent("onreadystatechange", temp_function);
	}
};
OSRM.Browser.onUnloadHandler = function( function_pointer, the_window ) {
	the_window = the_window || window;			// default document
	var the_document = the_window.document;
	
	if(the_window.addEventListener) {			// FF, CH, IE9+
		the_window.addEventListener("unload", function_pointer, false);
	}
	else if(the_document.attachEvent) {			// IE8-
		the_document.attachEvent("onunload", function_pointer);
	}
};