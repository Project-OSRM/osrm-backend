var selectedFeature = null;
var popupCloseTimer = null;
var popupToRemove = null;

function rightClick(e) {
	console.log("right click1");
	if(null == selectedFeature)
		return;
	//alert('rightclick at '+e.xy.x+','+e.xy.y);
	console.log("right click2");
}

function dblClick(e) {
	//alert('dblclick at '+e.xy.x+','+e.xy.y);
}

function dblRightClick(e) {
	//alert('dblrightclick at '+e.xy.x+','+e.xy.y);
}

function leftClick(e) {
	//set start and target via clicks
	var lonlat = map.getLonLatFromViewPortPx(e.xy);
	var markername = "";
	
/*	//routing shall not be done by left clicks
 * if (e.ctrlKey || e.altKey) {
		markername = "end";
		isEndPointSet = true;
	} else if(e.shiftKey) {
		markername = "start";
		isStartPointSet = true;
	}
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
	reroute();
	*/
}

function featureSelected(f) {
	if("viapoint" == f.feature.name) {
	//	console.log('selected ' + f.feature.name);
		selectedFeature = f.feature;
	}
}

function featureUnselected(f) {
	if("viapoint" == f.feature.name) {
	//	console.log('unselected ' + f.feature.name);
		selectedFeature = null;
	}
}

function createPopup(feature) {
	if("viapoint" != feature.name || ISDRAGGING)
		return;
		
	if(popupCloseTimer != null) {
		clearInterval(popupCloseTimer);
		popupCloseTimer = null;
		map.removePopup(popupToRemove);
		popupToRemove = null;
	}
	var location = feature.geometry.getBounds().getCenterLonLat().clone();
	location.lon; location.lat;
	feature.popup = new OpenLayers.Popup.Anchored("ViaPointInfo",
		location,
		new OpenLayers.Size(16,16),
		'<div><a href="javascript:removeViaPoint('+ feature.viaIndex +')"><img src="img/cancel_small.png"/></a></div>',
		null,
		false,
		destroyPopup );
	feature.popup.backgroundColor = 'transparent';
	feature.popup.fixedRelativePosition = true;
	feature.popup.relativePosition = "tr";
	map.addPopup(feature.popup, true);
}

function destroyPopup(feature) {
	if(feature.popup) {
		popupToRemove = feature.popup;
		popupCloseTimer = setTimeout("removePopup()",2000);
	}
}

function removePopup() {
	if(null == popupToRemove)
		return;
		
	map.removePopup(popupToRemove);
	popupToRemove = null;
	popupCloseTimer = null;
}

function removeViaPoint(index) {
	for(var i = 0; i < map.popups.length; i++) {
		map.removePopup(map.popups[i]);
	}
	viaPointsVector.splice(index, 1);
	reroute();
}
