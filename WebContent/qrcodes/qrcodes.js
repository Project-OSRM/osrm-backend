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

/*
The word "QR Code" is registered trademark of DENSO WAVE INCORPORATED.
http://www.denso-wave.com/qrcode/faqpatent-e.html

Coding ideas were taken from JavaScript-HTML5 QRCode Generator,
copyright (c) 2011 by Amanuel Tewolde, licensed under MIT license.
http://www.opensource.org/licenses/mit-license.php
*/


// initialization for QR Code window
// [qr code creation / error handling]

// build and show qr code
function createQRCode(text) {
	// settings
	var QRCodeVersion = 4;
	var dotsize = 6;
		
	// create qrcode
	// [when storing longer strings, verify if QRCodeVersion is still sufficient -> catch errors]
	var qrcode = new QRCode(QRCodeVersion, QRErrorCorrectLevel.H);
	qrcode.addData(text);
	qrcode.make();
	var qrsize = qrcode.getModuleCount();
		
	// HTML5 capable browsers
	if(!window.opener.OSRM.Browser.IE6_8) {
		// fill canvas		
		var canvas = document.createElement("canvas");
		canvas.setAttribute('height', dotsize*qrsize );
		canvas.setAttribute('width', dotsize*qrsize );

		var context = canvas.getContext('2d');
		for (var x = 0; x < qrsize; x++)
		for (var y = 0; y < qrsize; y++)  {
			if (qrcode.isDark(y, x))
				context.fillStyle = "rgb(0,0,0)";  
			else
				context.fillStyle = "rgb(255,255,255)";
			context.fillRect ( x*dotsize, y*dotsize, dotsize, dotsize);
		}
	
		// create png
		var image = document.createElement("img");
		image.id = "qrcode";	
		image.src = canvas.toDataURL("image/png");
		document.getElementById("qrcode-container").appendChild(image);

	// IE8...
	} else {
		// fill table
		var html = "";
		html += "<table class='qrcode'>";
		for (var y = 0; y < qrsize; y++) {
			html += "<tr>";
			for (var x = 0; x < qrsize; x++)  {
				if (qrcode.isDark(y, x))
					html += "<td class='black'/>";
				else
					html += "<td class='white'/>";
			}
			html += "</tr>";
		}
		html += "</table>";
		
		// add html to window
		document.getElementById("qrcode-container").innerHTML = html;
	}
	document.getElementById("qrcode-link").innerHTML = text;
}


// populate window with qrcode or with error message
function onLoad() {
	if( window.opener.OSRM.G.active_shortlink )
		createQRCode(window.opener.OSRM.G.active_shortlink);
	else
		createErrorMessage();	
}


//start populating the window when it is fully loaded - and only if it was loaded from OSRM 
if(window.opener && window.opener.OSRM)
	window.opener.OSRM.Browser.onLoadHandler( onLoad, window );
