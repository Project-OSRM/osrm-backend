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

// OSRM utility functions
// [mixed functions]


OSRM.Utils = {
		
// [human readabilty functions]

// human readable time
toHumanTime: function(seconds){
   seconds = parseInt(seconds);
   minutes = parseInt(seconds/60);
   seconds = seconds%60;
   hours = parseInt(minutes/60);
   minutes = minutes%60;
   if(hours==0 && minutes==0){ return seconds + '&nbsp;' + 's'; }
   else if(hours==0){ return minutes + '&nbsp;' + 'min'; }
   else{ return hours + '&nbsp;' + 'h' + '&nbsp;' + minutes + '&nbsp;' + 'min';}
},
//human readable distance
toHumanDistanceMeters: function(meters){
	var distance = parseInt(meters);
	
	distance = distance / 1000;
	if(distance >= 100){ return (distance).toFixed(0)+'&nbsp;' + 'km'; }
	else if(distance >= 10){ return (distance).toFixed(1)+'&nbsp;' + 'km'; }
	else if(distance >= 0.1){ return (distance).toFixed(2)+'&nbsp;' + 'km'; }
	else{ return (distance*1000).toFixed(0)+'&nbsp;' + 'm'; }		
},
toHumanDistanceMiles: function(meters){
	var distance = parseInt(meters);
	
	distance = distance / 1609.344;
	if(distance >= 100){ return (distance).toFixed(0)+'&nbsp;' + 'mi'; }
	else if(distance >= 10){ return (distance).toFixed(1)+'&nbsp;' + 'mi'; }
	else if(distance >= 0.1){ return (distance).toFixed(2)+'&nbsp;' + 'mi'; }
	else{ return (distance*5280).toFixed(0)+'&nbsp;' + 'ft'; }
},
toHumanDistance: null,


// [verification routines]

// verify angles
isLatitude: function(value) {
	if( value >=-90 && value <=90)
		return true;
	else
		return false;
},
isLongitude: function(value) {
	if( value >=-180 && value <=180)
		return true;
	else
		return false;
},
isNumber: function(n) {
	  return !isNaN(parseFloat(n)) && isFinite(n);
}

};