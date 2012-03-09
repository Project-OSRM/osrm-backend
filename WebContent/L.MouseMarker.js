L.MouseMarker = L.Marker.extend({
	initialize: function (latlng, options) {
		L.Marker.prototype.initialize.apply(this, arguments);
	},

//	_initInteraction: function (){
//		L.Marker.prototype._initInteraction.apply(this, arguments);
// 		if (this.options.clickable)
//			L.DomEvent.addListener(this._icon, 'mousemove', this._fireMouseEvent, this);
//	},

//	_fireMouseEvent: function (e) {
//		this.fire(e.type, {
//			latlng: this._map.mouseEventToLatLng(e),
//			layerPoint: this._map.mouseEventToLayerPoint(e)
//		});		
//		L.DomEvent.stopPropagation(e);
//	},
	
	_onMouseClick: function (e) {
		L.DomEvent.stopPropagation(e);
		if (this.dragging && this.dragging.moved()) { return; }
		this.fire(e.type, {
			altKey: e.altKey,
			ctrlKey: e.ctrlKey,
			shiftKey: e.shiftKey,
			button: e.button
		});
	},	
});