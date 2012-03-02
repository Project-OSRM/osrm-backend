/*
 *  Open Source Routing Machine (OSRM) - Web (GUI) Interface
 *	Copyright (C) Pascal Neis, 2011
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU AFFERO General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU Affero General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	or see http://www.gnu.org/licenses/agpl.txt.
 */
 
/**
 * Title: Geocode.js
 * Description: JS file for geocoding
 *
 * @author Pascal Neis, pascal@neis-one.org 
 * @version 0.1 2011-05-15
 */

//======================
// OBJECTS
var HOST_GEOCODE_URL = 'http://open.mapquestapi.com';
var GEOCODE_POST = HOST_GEOCODE_URL + '/nominatim/v1/search?format=json&json_callback=showResultsGeocode';
var searchType = 'search';

var isStartPointSet = false;
var isEndPointSet = false;

//======================
// FUNCTIONS
/*
 * geocodeAddress()-Function to read out textfield and send request to a OSM nominatim geocoder
 */
function geocodeAddress(tf){
	var freeform;
		
	if(tf == 'start'){
		freeform = document.getElementById('tfStartSearch').value;
	}
	if(tf == 'end'){
		freeform = document.getElementById('tfEndSearch').value;
	}

	if(freeform.match(/^\s*[-+]?[0-9]*\.?[0-9]+\s*[,;]\s*[-+]?[0-9]*\.?[0-9]+\s*$/)){
		
		var marker;
		if(tf == 'start'){
			isStartPointSet = true;
			marker = 'start';
		}
		if(tf == 'end'){
			isEndPointSet = true;
			marker = 'end';
		}
		var coord = freeform.split(/[,;]/);
                lonlat = new OpenLayers.LonLat(coord[1],coord[0]);
                setMarkerAndZoom(marker, lonlat);
                return;
	}

	document.getElementById('information').style.visibility = 'visible';
	document.getElementById('information').innerHTML =  '<p class="infoHL">One moment please ...</p>';


    var newURL = GEOCODE_POST + "&q="+freeform;
	var script = document.createElement('script');
    script.type = 'text/javascript';
    script.src = newURL;
    document.body.appendChild(script);
}

/*
 * showResultsGeocode()-Function to show the geocode result in a div
 */
function showResultsGeocode(response) {
    var html = '';    
    var lonlat = '';
	
	if(response){
		html += '<p class="infoHL">Search Results:</p>';
		html += '<table>'
		var markername;
		if(getStatus()=='start'){
			markername = 'start'; isStartPointSet = true;
		}
		else if(getStatus()=='end'){
			markername = 'end';	isEndPointSet = true;
		}
		
		for(var i=0; i < response.length; i++){
			var result = response[i]; var resultNum = i+1;			
        	//odd or even ?
        	var rowstyle='geocodeResultOdd';
        	if(i%2==0){ rowstyle='geocodeResultEven'; }
        	
			html += '<tr class="'+rowstyle+'">';
			html += '<td align="right" valign="top"><span class="routeSummarybold">'+resultNum+'</span></td>';
			html += '<td class="'+rowstyle+'">';
			if(result.display_name){
				var new_display_name = result.display_name;//.replace(/,/g, ",<br />")
				html += '<a href="#" onclick="javascript:setMarkerAndZoom(\''+markername+'\', new OpenLayers.LonLat('+result.lon+','+result.lat+'));">'+new_display_name.trim()+'</a>';
			}
			html += "</td></tr>";
			
			//alert(result.lat + ", " + result.lon);
			if(lonlat == ''){
				lonlat = new OpenLayers.LonLat(result.lon,result.lat);
			}
		}		
		html += '</table>';
		
		setMarkerAndZoom(markername, lonlat);
	}
	
    
    switch (searchType) {
		case "search":
			document.getElementById('information').style.display = "";
			document.getElementById('information').innerHTML = html;
			break;
	}
}

/*
 * setMarkerAndZoom()-Function to set a marker on the map and zoom
 */
function setMarkerAndZoom(markername,lonlat){
	setMarker(markername,lonlat);
	
	//Hack - FIXME !
	lonlat.lon -= 500;
	//Set Center
	map.setCenter(lonlat, 15);
}

/*
 * setMarker()-Function to set a marker on the map
 */
function setMarker(markername,lonlat){
	lonlat.transform(EPSG_4326, EPSG_900913);
	for(var i= 0; i<dragLayer.features.length; i++){
		if(dragLayer.features[i].name == markername){ dragLayer.removeFeatures([dragLayer.features[i]]); }	
	}
	var point = new OpenLayers.Feature.Vector(new OpenLayers.Geometry.Point(lonlat.lon, lonlat.lat));
	point.attributes = { icon: "img/start.png" };

	if(markername == 'start'){
		point.attributes = { icon: "img/start.png" };
	}
	else if(markername == 'end'){
		point.attributes = { icon: "img/end.png" };
	}
		
	point.name = markername;
	dragLayer.addFeatures([point]);
}

/*
 * getMarkerByName()-Function to return the marker-object by a name
 */
function getMarkerByName(markerName){
	for(var i= 0; i<dragLayer.features.length; i++){
		if(dragLayer.features[i].name == markerName){
			return dragLayer.features[i];
		}
	}
}
