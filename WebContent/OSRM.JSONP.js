// OSRM JSONP call wrapper 
// w/ DOM cleaning, fencing, timout handling

OSRM.JSONP = {
	fences: {},
	callbacks: {},
	timeouts: {},
	timers: {},
	
	TIMEOUT: OSRM.DEFAULTS.JSONP_TIMEOUT,
	
	late: function() { console.log("reply too late");},
	empty: function() { console.log("empty callback");},
		
	call: function(source, callback_function, timeout_function, timeout, id) {
		// only one active JSONP call per id
		if (OSRM.JSONP.fences[id] == true)
			return false;
		OSRM.JSONP.fences[id] = true;
		
//		console.log("[status] jsonp init for "+id);
//		console.log("[status] jsonp request ",source);		
				
		// wrap timeout function
		OSRM.JSONP.timeouts[id] = function(response) {
			timeout_function(response);
			
//			var jsonp = document.getElementById('jsonp_'+id);		// clean DOM
//			if(jsonp)
//				jsonp.parentNode.removeChild(jsonp);
			OSRM.JSONP.callbacks[id] = OSRM.JSONP.late;				// clean functions
			OSRM.JSONP.timeouts[id] = OSRM.JSONP.late;
			OSRM.JSONP.fences[id] = undefined;						// clean fence
			
//			console.log("timeout: "+id); 							// at the end - otherwise racing conditions may happen
//			document.getElementById('information-box').innerHTML += "timeout:" + id + "<br>";			
		};
		
		// wrap callback function
		OSRM.JSONP.callbacks[id] = function(response) {
			clearTimeout(OSRM.JSONP.timers[id]);					// clear timeout timer
			OSRM.JSONP.timers[id] = undefined;
			
			if( OSRM.JSONP.fences[id] == undefined )				// fence to prevent execution after timeout function (when precompiled!)
				return;		

			callback_function(response);							// actual wrapped callback 
			
//			var jsonp = document.getElementById('jsonp_'+id);		// clean DOM
//			if(jsonp)
//				jsonp.parentNode.removeChild(jsonp);
			OSRM.JSONP.callbacks[id] = OSRM.JSONP.late;				// clean functions
			OSRM.JSONP.timeouts[id] = OSRM.JSONP.late;
			OSRM.JSONP.fences[id] = undefined;						// clean fence
			
//			console.log("[status] jsonp response for "+id);			// at the end - otherwise racing conditions may happen
//			document.getElementById('information-box').innerHTML += "callback:" + id + "<br>";
		};
		
		// clean DOM (cannot reuse script element with all browsers, unfortunately)
		var jsonp = document.getElementById('jsonp_'+id);
		if(jsonp)
			jsonp.parentNode.removeChild(jsonp);		
		
		// add script to DOM
		var script = document.createElement('script');
		script.type = 'text/javascript';
		script.id = 'jsonp_'+id;
		script.src = source + "&json_callback=OSRM.JSONP.callbacks."+id + "&jsonp=OSRM.JSONP.callbacks."+id;
		document.head.appendChild(script);
		
		// start timeout timer
		OSRM.JSONP.timers[id] = setTimeout(OSRM.JSONP.timeouts[id], timeout);
		
		return true;
	}
};