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
secondsToTime: function(seconds){
   seconds = parseInt(seconds);
   minutes = parseInt(seconds/60);
   seconds = seconds%60;
   hours = parseInt(minutes/60);
   minutes = minutes%60;
   if(hours==0){
   	return minutes + '&nbsp;' + 'min';
   }
   else{
   	return hours + '&nbsp;' + 'h' + '&nbsp;' + minutes + '&nbsp;' + 'min';
   }
},
//human readable distance
metersToDistance: function(distance){
	distance = parseInt(distance);
	
	if(distance >= 100000){ return (parseInt(distance/1000))+'&nbsp;' + 'km'; }
	else if(distance >= 10000){ return (parseInt(distance/1000).toFixed(1))+'&nbsp;' + 'km'; }
	else if(distance >= 1000){ return (parseFloat(distance/1000).toFixed(2))+'&nbsp;' + 'km'; }
	else{ return distance+'&nbsp;' + 'm'; }
},


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
}

};