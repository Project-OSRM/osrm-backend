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

// OSRM browser detection
// [simple detection routines to respect some browser peculiarities] 


(function() {
	var useragent = navigator.userAgent;
	
	OSRM.Browser = {
 		FF3:	useragent.search(/Firefox\/3/),
		IE6_9:	useragent.search(/MSIE (6|7|8|9)/)
	};
}());

// (runs anonymous function to prevent local variables cluttering global namespace)