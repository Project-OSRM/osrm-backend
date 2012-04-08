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
		
supported_languages: ["en", "de"],		
		
// initialize localization
init: function() {
	// create dropdown menu
	var select = document.createElement('select');
	select.id = "language-toggle";
	select.onchange = function() { OSRM.Localization.change(this.value); };
	
	// fill dropdown menu
	for(var i=0, size=OSRM.Localization.supported_languages.length; i<size; i++) {
		var option=document.createElement("option");
		option.innerHTML = OSRM.Localization.supported_languages[i];
		option.value = OSRM.Localization.supported_languages[i];
		select.appendChild(option);
	}
	
	// add element to DOM
	var input_mask_header = document.getElementById('input-mask-header'); 
	input_mask_header.insertBefore(select,input_mask_header.firstChild);
	
	// initialize default language
	OSRM.Localization.change( OSRM.DEFAULTS.LANGUAGE );
},

// perform language change
change: function(language) {
	OSRM.DEFAULTS.LANGUAGE = language;
	if( OSRM.Localization[language]) {
		OSRM.GUI.setLanguage();
		if( document.getElementById('information-box').innerHTML != "" )
			OSRM.RoutingDescription.show( OSRM.G.response );
	} else {
		var script = document.createElement('script');
		script.type = 'text/javascript';
		script.src = "Localization/OSRM.Locale."+language+".js";
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