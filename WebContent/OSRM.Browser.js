// OSRM browser detection


(function() {
	var useragent = navigator.userAgent;
	
	OSRM.Browser = {
 		FF3:	useragent.search(/Firefox\/3/),
		IE6_8:	useragent.search(/MSIE (6|7|8)/),
	};
}());

// (runs anonymous function to prevent local variables cluttering global namespace)