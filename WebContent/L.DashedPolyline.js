// dashed polyline
L.DashedPolyline = L.Polyline.extend({
	initialize: function(latlngs, options) {
		L.Polyline.prototype.initialize.call(this, latlngs, options);
	},
	
	options: {
		dashed: true
	},
});


// svg rendering
L.DashedPolyline = !L.Browser.svg ? L.DashedPolyline : L.DashedPolyline.extend({
	_updateStyle: function () {
		L.Polyline.prototype._updateStyle.call(this);
		if (this.options.stroke) {
			if (this.options.dashed == true)
				this._path.setAttribute('stroke-dasharray', '8,6');
			else
				this._path.setAttribute('stroke-dasharray', '');
		}
	},
});


// vml rendering
L.DashedPolyline = L.Browser.svg || !L.Browser.vml ? L.DashedPolyline : L.DashedPolyline.extend({
	_updateStyle: function () {
		L.Polyline.prototype._updateStyle.call(this);
		if (this.options.stroke) {
			if (this.options.dashed == true)
				this._stroke.dashstyle = "dash";
			else
				this._stroke.dashstyle = "solid";
		}
	},
	
});
