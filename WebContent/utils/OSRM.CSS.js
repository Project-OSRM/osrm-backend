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

// OSRM CSS manipulator
// [edit css styles]

OSRM.CSS = {
	getStylesheet: function(filename, the_document) {
		the_document = the_document || document;
		var stylesheets = the_document.styleSheets;
		for(var i=0, size=stylesheets.length; i<size; i++) {
			if( stylesheets[i].href.indexOf(filename) >= 0)
				return stylesheets[i];
		}
		return null;
	},
	
	insert: function(stylesheet, selector, rule) {
		if( stylesheet.addRule ){
			stylesheet.addRule(selector, rule);
		} else if( stylesheet.insertRule ){
            stylesheet.insertRule(selector + ' { ' + rule + ' }', stylesheet.cssRules.length);
		}
	}		
};