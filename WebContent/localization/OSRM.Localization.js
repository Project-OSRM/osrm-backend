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

// OSRM localization
// [basic localization options]


OSRM.Localization = {
		
supported_languages: [ {display_name:"en", encoding:"en"},
                       {display_name:"de", encoding:"de"}
],		
		
// initialize localization
init: function() {
	// create dropdown menu
	var select = document.createElement('select');
	select.id = "gui-language-toggle";
	//select.className = "top-left-button";
	select.onchange = function() { OSRM.Localization.change(this.value); };
	
	// fill dropdown menu
	for(var i=0, size=OSRM.Localization.supported_languages.length; i<size; i++) {
		var option=document.createElement("option");
		option.innerHTML = OSRM.Localization.supported_languages[i].display_name;
		option.value = OSRM.Localization.supported_languages[i].encoding;
		select.appendChild(option);
	}
	
	// add element to DOM
	var input_mask_header = document.getElementById('input-mask-header'); 
	input_mask_header.insertBefore(select,input_mask_header.firstChild);
	
	// create visible dropdown menu
	var textnode = document.createTextNode(OSRM.DEFAULTS.LANGUAGE);
	var myspan = document.createElement("span");
	myspan.className = "styled-select";
	myspan.id = "styled-select" + select.id;
	myspan.appendChild(textnode);
	select.parentNode.insertBefore(myspan, select);
	myspan.style.width = (select.clientWidth-2)+"px";
	myspan.style.height = (select.clientHeight)+"px";
	
	// initialize default language
	OSRM.Localization.change( OSRM.DEFAULTS.LANGUAGE );
},

// perform language change
change: function(language) {
	OSRM.DEFAULTS.LANGUAGE = language;
	// update selector
	var select = document.getElementById('gui-language-toggle');
	var option = select.getElementsByTagName("option");
	select.value = language;
	for(var i = 0; i < option.length; i++)
		if(option[i].selected == true) {
			document.getElementById("styled-select" + select.id).childNodes[0].nodeValue = option[i].childNodes[0].nodeValue;
			break;
		}	
	// change gui language
	if( OSRM.Localization[language]) {
		OSRM.GUI.setLanguage();
		if( OSRM.G.markers.route.length > 1)
			OSRM.Routing.getRoute();
		else if(OSRM.G.markers.route.length > 0 && document.getElementById('information-box').innerHTML != "" ) {
			OSRM.Geocoder.call( OSRM.C.SOURCE_LABEL, document.getElementById("gui-input-source").value );
			OSRM.Geocoder.call( OSRM.C.TARGET_LABEL, document.getElementById("gui-input-target").value );
		} else {
			OSRM.Geocoder.updateAddress(OSRM.C.SOURCE_LABEL, false);
			OSRM.Geocoder.updateAddress(OSRM.C.TARGET_LABEL, false);
			document.getElementById('information-box').innerHTML = "";
			document.getElementById('information-box-header').innerHTML = "";			
		}
	} else {
		var script = document.createElement('script');
		script.type = 'text/javascript';
		script.src = "localization/OSRM.Locale."+language+".js";
		document.head.appendChild(script);		
	}
},
		
// if existing, return localized string -> English string -> input string
translate: function(text) {
	if( OSRM.Localization[OSRM.DEFAULTS.LANGUAGE] && OSRM.Localization[OSRM.DEFAULTS.LANGUAGE][text] )
		return OSRM.Localization[OSRM.DEFAULTS.LANGUAGE][text];
	else if( OSRM.Localization["en"] && OSRM.Localization["en"][text] )
		return OSRM.Localization["en"][text];
	else
		return text;
}
};

// shorter call to translate function
OSRM.loc = OSRM.Localization.translate;