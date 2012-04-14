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

current_language: OSRM.DEFAULTS.LANGUAGE,
		
// initialize localization
init: function() {
	// create dropdown menu
	var select = document.createElement('select');
	select.id = "gui-language-toggle";
	select.className = "top-left-button";
	select.onchange = function() { OSRM.Localization.setLanguage(this.value); };	
		
	// fill dropdown menu
	var supported_languages = OSRM.DEFAULTS.LANGUAGE_SUPPORTED;
	for(var i=0, size=supported_languages.length; i<size; i++) {
		var option=document.createElement("option");
		option.innerHTML = supported_languages[i].display_name;
		option.value = supported_languages[i].encoding;
		select.appendChild(option);
	}
	select.value = OSRM.DEFAULTS.LANGUAGE; 
	
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
},

// perform language change
setLanguage: function(language) {
	if( OSRM.Localization[language]) {
		OSRM.Localization.current_language = language;
		// update selector
		if( select = document.getElementById('gui-language-toggle') ) {		// yes, = not == !
			var option = select.getElementsByTagName("option");
			select.value = language;
			for(var i = 0; i < option.length; i++)
				if(option[i].selected == true) {
					document.getElementById("styled-select" + select.id).childNodes[0].nodeValue = option[i].childNodes[0].nodeValue;
					break;
				}
		}		
		// change gui language		
		OSRM.GUI.setLabels();
		// requery data
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
	} else if(OSRM.DEFAULTS.LANUGAGE_ONDEMAND_RELOADING==true) {
		var supported_languages = OSRM.DEFAULTS.LANGUAGE_SUPPORTED;
		for(var i=0, size=supported_languages.length; i<size; i++) {
			if( supported_languages[i].encoding == language) {
				var script = document.createElement('script');
				script.type = 'text/javascript';
				script.src = OSRM.DEFAULTS.LANGUAGE_FILES_DIRECTORY + "OSRM.Locale."+language+".js";
				document.head.appendChild(script);
				break;
			}
		}		
	}
},
		
// if existing, return localized string -> English string -> input string
translate: function(text) {
	if( OSRM.Localization[OSRM.Localization.current_language] && OSRM.Localization[OSRM.Localization.current_language][text] )
		return OSRM.Localization[OSRM.Localization.current_language][text];
	else if( OSRM.Localization[OSRM.DEFAULTS.LANGUAGE] && OSRM.Localization[OSRM.DEFAULTS.LANGUAGE][text] )
		return OSRM.Localization[OSRM.DEFAULTS.LANGUAGE][text];
	else
		return text;
}
};

// shorter call to translate function
OSRM.loc = OSRM.Localization.translate;