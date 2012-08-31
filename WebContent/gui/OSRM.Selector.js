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

// OSRM selector
// [create special selector elements]


OSRM.GUI.extend( {

// initialize selector with all options and our look&feel
selectorInit: function(id, options, selected, onchange_fct) {
	// create dropdown menu
	var select = document.getElementById(id);
	select.className += " styled-select-helper base-font";
	select.onchange = function() { OSRM.GUI._selectorOnChange(this); onchange_fct(this.value); };	
		
	// fill dropdown menu
	for(var i=0, size=options.length; i<size; i++) {
		var option=document.createElement("option");
		option.innerHTML = options[i].display;
		option.value = options[i].value;
		select.appendChild(option);
	}
	select.value = options[selected].value;
	
	// create visible dropdown menu
	var textnode = document.createTextNode( options[selected].display );
	var myspan = document.createElement("span");
	myspan.className = "styled-select base-font";
	myspan.id = "styled-select-" + select.id;
	myspan.appendChild(textnode);
	select.parentNode.insertBefore(myspan, select);
	myspan.style.width = (select.offsetWidth-2)+"px";
	myspan.style.height = (select.offsetHeight)+"px";	// clientHeight gives the height of the opened dropbox!
},

// required behaviour of selector on change to switch shown name
_selectorOnChange: function(select) {
	var option = select.getElementsByTagName("option");	
	for(var i = 0; i < option.length; i++)
	if(option[i].selected == true) {
		document.getElementById("styled-select-" + select.id).childNodes[0].nodeValue = option[i].childNodes[0].nodeValue;
		break;
	}
},

// change selector value
selectorChange: function(id, value) {
	var select = document.getElementById(id);
	select.value = value;
	OSRM.GUI._selectorOnChange(select);
},

// replace selector options with new names
selectorRenameOptions: function(id, options) {
	var select = document.getElementById(id);
	var styledSelect = document.getElementById("styled-select-"+id);

	// create new dropdown menu
	var new_select = document.createElement("select");
	new_select.id = id;
	new_select.className = select.className;
	new_select.onchange = select.onchange;	
		
	// fill new dropdown menu
	var selected_display = "";
	for(var i=0, size=options.length; i<size; i++) {
		var option=document.createElement("option");
		option.innerHTML = options[i].display;
		option.value = options[i].value;
		new_select.appendChild(option);
		
		if( options[i].value == select.value )
			selected_display = options[i].display;		
	}
	new_select.value = select.value;
	
	// switch old with new dropdown menu
	select.parentNode.insertBefore(new_select, select);
	select.parentNode.removeChild(select);
	
	// change styled dropdown menu size & language
	styledSelect.childNodes[0].nodeValue = selected_display;
	styledSelect.style.width = (new_select.offsetWidth-2)+"px";
	styledSelect.style.height = (new_select.offsetHeight)+"px";
	
//	// old variant without creating a new dropdown menu (works in current browsers, but not in older FF or IE)
//	var select = document.getElementById(id);
//	var select_options = select.getElementsByTagName("option");
//	var styledSelect = document.getElementById("styled-select-"+id);
//	
//	// fill dropdown menu with new option names
//	for(var i = 0; i < select_options.length; i++) {
//		select_options[i].childNodes[0].nodeValue = options[i].display;
//		
//		if(select_options[i].selected == true)
//			styledSelect.childNodes[0].nodeValue = options[i].display;
//	}
//	
//	// resize visible dropdown menu as needed
//	styledSelect.style.width = (select.offsetWidth-2)+"px";
//	styledSelect.style.height = (select.offsetHeight)+"px";	// clientHeight gives the height of the opened dropbox!	
}

});