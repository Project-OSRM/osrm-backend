// OSRM browser detection


(function() {
	var useragent = navigator.userAgent;
	
	OSRM.Browser = {
 		FF3:	useragent.search(/Firefox\/3/),
		IE6_9:	useragent.search(/MSIE (6|7|8|9)/),
	};
}());

// (runs anonymous function to prevent local variables cluttering global namespace)