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

// OSRM old browser support
// [simple browser detection and routines to support some old browsers] 


// browser detection (runs anonymous function to prevent local variables cluttering global namespace)
(function() {
	var useragent = navigator.userAgent;
	
	OSRM.Browser = {
 		FF3:	useragent.search(/Firefox\/3/),
		IE6_9:	useragent.search(/MSIE (6|7|8|9)/)
	};
}());


// compatibility tools for old browsers
function getElementsByClassName(node, classname) {
    var a = [];
    var re = new RegExp('(^| )'+classname+'( |$)');
    var els = node.getElementsByTagName("*");
    for(var i=0,j=els.length; i<j; i++)
        if(re.test(els[i].className))a.push(els[i]);
    return a;
}
document.head = document.head || document.getElementsByTagName('head')[0];