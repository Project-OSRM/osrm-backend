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

// default directory for localization files
DIRECTORY: "localization/",

// holds currently active language
current_language: OSRM.DEFAULTS.LANGUAGE,

//initialize localization
init: function() {
	// fill option list and find default entry
	var options = [];
	var options_2 = [];
	var selected = 0;	
	var supported_languages = OSRM.DEFAULTS.LANGUAGE_SUPPORTED;
	for(var i=0, size=supported_languages.length; i<size; i++) {
		options.push( {display:supported_languages[i].encoding, value:supported_languages[i].encoding} );
		options_2.push( {display:supported_languages[i].name, value:supported_languages[i].encoding} );
		if( supported_languages[i].encoding == OSRM.DEFAULTS.LANGUAGE )
			selected=i;
	}
	
	// generate selectors
	OSRM.GUI.selectorInit("gui-language-toggle", options, selected, OSRM.Localization.setLanguageWrapper);
	OSRM.GUI.selectorInit("gui-language-2-toggle", options_2, selected, OSRM.Localization.setLanguageWrapper);
	
	// set default language
	OSRM.Localization.setLanguage( OSRM.DEFAULTS.LANGUAGE );
},
setLanguageWrapper: function(language) {		// wrapping required to correctly prevent localization tooltip from showing
	OSRM.GUI.deactivateTooltip( "LOCALIZATION" );
	OSRM.Localization.setLanguage(language);
},
setLanguage: function(language) {
	// change value of both language selectors
	OSRM.GUI.selectorChange( 'gui-language-toggle', language );
	OSRM.GUI.selectorChange( 'gui-language-2-toggle', language );
	
	if( OSRM.Localization[language]) {
		OSRM.Localization.current_language = language;
		// change gui language		
		OSRM.GUI.setLabels();
		// change map language
		for(var i=0, size=OSRM.G.localizable_maps.length; i<size; i++) {
			OSRM.G.localizable_maps[i].options.culture = OSRM.loc("CULTURE");
		}
		if(OSRM.G.map.layerControl.getActiveLayer().redraw)
			OSRM.G.map.layerControl.getActiveLayer().redraw();
		// requery data
		if( OSRM.G.markers == null )
			return;
		if( OSRM.G.markers.route.length > 1)
			OSRM.Routing.getRoute({keepAlternative:true});
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
				script.src = OSRM.Localization.DIRECTORY+"OSRM.Locale."+language+".js";
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
