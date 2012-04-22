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

// Leaflet bugfixes
// [assorted bugfixes to Leaflet functions we use]


// return closest point on segment or distance to that point
L.LineUtil._sqClosestPointOnSegment = function (p, p1, p2, sqDist) {
	var x = p1.x,
		y = p1.y,
		dx = p2.x - x,
		dy = p2.y - y,
		dot = dx * dx + dy * dy,
		t;

	if (dot > 0) {
		t = ((p.x - x) * dx + (p.y - y) * dy) / dot;

		if (t > 1) {
			x = p2.x;
			y = p2.y;
		} else if (t > 0) {
			x += dx * t;
			y += dy * t;
		}
	}

	dx = p.x - x;
	dy = p.y - y;
		
	// DS_CHANGE: modified return values
	if(sqDist)
		return dx*dx + dy*dy;
	else {
		var p = new L.Point(x,y);
		p._sqDist = dx*dx + dy*dy;
		return p;
	}
};


// makes requestAnimFrame respect the immediate paramter -> prevents drag events after dragend events
// (alternatively: add if(!this.dragging ) return to L.Draggable._updatePosition, but must be done in leaflet.js!)
// [TODO: In Leaflet 0.4 use L.Util.cancelAnimFrame(this._animRequest) in L.Draggable._onUp() instead, also has to be done in leaflet.js!]
L.Util.requestAnimFrame = (function () {
	function timeoutDefer(callback) {
		window.setTimeout(callback, 1000 / 60);
	}

	var requestFn = window.requestAnimationFrame ||
		window.webkitRequestAnimationFrame ||
		window.mozRequestAnimationFrame ||
		window.oRequestAnimationFrame ||
		window.msRequestAnimationFrame ||
		timeoutDefer;

	return function (callback, context, immediate, contextEl) {
		callback = context ? L.Util.bind(callback, context) : callback;
		if (immediate ) {		// DS_CHANGE: removed additional condition requestFn === timeoutDefer
			callback();
		} else {
			requestFn(callback, contextEl);
		}
	};
}());	
