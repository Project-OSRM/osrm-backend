L.BingLayer = L.TileLayer.extend({
	options: {
		subdomains: [0, 1, 2, 3],
		type: 'Aerial',
		attribution: 'Bing',
		culture: 'en-US'
	},

	initialize: function(key, options) {
		L.Util.setOptions(this, options);

		this._key = key;
		this._url = null;
		this.meta = {};
		this.loadMetadata();
	},

	tile2quad: function(x, y, z) {
		var quad = '';
		for (var i = z; i > 0; i--) {
			var digit = 0;
			var mask = 1 << (i - 1);
			if ((x & mask) != 0) digit += 1;
			if ((y & mask) != 0) digit += 2;
			quad = quad + digit;
		}
		return quad;
	},

	getTileUrl: function(p, z) {
		var z = this._getZoomForUrl();
		var subdomains = this.options.subdomains,
			s = this.options.subdomains[(p.x + p.y) % subdomains.length];
		return this._url.replace('{subdomain}', s)
				.replace('{quadkey}', this.tile2quad(p.x, p.y, z))
				.replace('{culture}', this.options.culture);
	},

	loadMetadata: function() {
		var _this = this;
		var cbid = '_bing_metadata_' + L.Util.stamp(this);
		window[cbid] = function (meta) {
			_this.meta = meta;
			window[cbid] = undefined;
			var e = document.getElementById(cbid);
			e.parentNode.removeChild(e);
			if (meta.errorDetails) {
				alert("Got metadata" + meta.errorDetails);
				return;
			}
			_this.initMetadata();
		};
		var url = "http://dev.virtualearth.net/REST/v1/Imagery/Metadata/" + this.options.type + "?include=ImageryProviders&jsonp=" + cbid + "&key=" + this._key;
		var script = document.createElement("script");
		script.type = "text/javascript";
		script.src = url;
		script.id = cbid;
		document.getElementsByTagName("head")[0].appendChild(script);
	},

	initMetadata: function() {
		var r = this.meta.resourceSets[0].resources[0];
		this.options.subdomains = r.imageUrlSubdomains;
		this._url = r.imageUrl;
		this._providers = [];
		for (var i = 0; i < r.imageryProviders.length; i++) {
			var p = r.imageryProviders[i];
			for (var j = 0; j < p.coverageAreas.length; j++) {
				var c = p.coverageAreas[j];
				var coverage = {zoomMin: c.zoomMin, zoomMax: c.zoomMax, active: false};
				var bounds = new L.LatLngBounds(
						new L.LatLng(c.bbox[0]+0.01, c.bbox[1]+0.01),
						new L.LatLng(c.bbox[2]-0.01, c.bbox[3]-0.01)
				);
				coverage.bounds = bounds;
				coverage.attrib = p.attribution;
				this._providers.push(coverage);
			}
		}
		this._update();
	},

	_update: function() {
		if (this._url == null || !this._map) return;
		this._update_attribution();
		L.TileLayer.prototype._update.apply(this, []);
	},

	_update_attribution: function() {
		var bounds = this._map.getBounds();
		var zoom = this._map.getZoom();
		for (var i = 0; i < this._providers.length; i++) {
			var p = this._providers[i];
			if ((zoom <= p.zoomMax && zoom >= p.zoomMin) &&
					bounds.intersects(p.bounds)) {
				if (!p.active)
					this._map.attributionControl.addAttribution(p.attrib);
				p.active = true;
			} else {
				if (p.active)
					this._map.attributionControl.removeAttribution(p.attrib);
				p.active = false;
			}
		}
	},

	onRemove: function(map) {
		for (var i = 0; i < this._providers.length; i++) {
			var p = this._providers[i];
			if (p.active) {
				this._map.attributionControl.removeAttribution(p.attrib);
				p.active = false;
			}
		}
        	L.TileLayer.prototype.onRemove.apply(this, [map]);
	}
});



/* 
 * Portions of this code and logic copied from OpenLayers and
 * redistributed under the original Clear BSD license terms:
 * 
 * http://trac.osgeo.org/openlayers/browser/license.txt
 *
 * Copyright 2005-2010 OpenLayers Contributors, released under 
 * the Clear BSD license. See authors.txt for a list of contributors.
 * All rights reserved.
 *
 * --
 *
 * Leaflet-specific modifications are released under the following
 * terms:
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. 
 */

L.TileLayer.Bing = L.TileLayer.extend({
  
  supportedTypes: ['Road', 'Aerial', 'AerialWithLabels'],
  
  attributionTemplate: '<span style="display:inline-block">' +
       '<a target="_blank" href="http://www.bing.com/maps/">' +
       //'<img src="{logo}" /></a><br><span>{copyrights}' +
       '</a><span>{copyrights}' +
       '<a style="white-space: nowrap" target="_blank" '+
       'href="http://www.microsoft.com/maps/product/terms.html">' +
       'Terms of Use</a></span></span>',
       
  supportedCultures: {"en":"en-US", "de":"de-DE", "fr":"fr-FR", "it":"it-IT", "es":"es-ES", "nl":"nl-BE"},
  
  initialize: function(/*String*/ apiKey, /*String*/ mapType, /*Object*/ options) {
    
    this._apiKey = apiKey;
    this._mapType = mapType;
    
    this._loadMetadata();
    
    L.Util.setOptions(this, options);
  },
  
  redraw: function() {
    this._reset();
    this._update();  
  },
  
  _loadMetadata: function() {
    this._callbackId = "_l_tilelayer_bing_" + (L.TileLayer.Bing._callbackId++);
    var that = this;
    window[this._callbackId] = function() {
      L.TileLayer.Bing.processMetadata.apply(that, arguments);
    };
    
    var params = {
      key: this._apiKey,
      jsonp: this._callbackId,
      include: 'ImageryProviders'
    },
        url = "http://dev.virtualearth.net/REST/v1/Imagery/Metadata/" +
              this._mapType + L.Util.getParamString(params),
        script = document.createElement("script");
        
    script.type = "text/javascript";
    script.src = url;
    script.id = this._callbackId;
    document.getElementsByTagName("head")[0].appendChild(script);
  },
  
  _onMetadataLoaded: function() {},
  
  onAdd: function(map, insertAtTheBottom) {
    if (!this.metadata) {
      this._onMetadataLoaded = L.Util.bind(function() {
        L.TileLayer.prototype.onAdd.call(this, map, insertAtTheBottom);
        map.on('moveend', this._updateAttribution, this);
        this._updateAttribution();
      }, this);
    } else {
      L.TileLayer.prototype.onAdd.call(this, map, insertAtTheBottom);
      map.on('moveend', this._updateAttribution, this);
      this._updateAttribution();
    }
  },
  
  onRemove: function(map) {
    if (this._map.attributionControl) {
      this._map.attributionControl.removeAttribution(this.attribution);
    }
    this._map.off('moveend', this._updateAttribution, this);
    L.TileLayer.prototype.onRemove.call(this, map);
  },
  
  getTileUrl: function(xy, z) {
    var subdomains = this.options.subdomains,
			  quadDigits = [],
			  i = z,
			  digit,
			  mask,
			  quadKey;
    // borrowed directly from OpenLayers
    for (; i > 0; --i) {
        digit = '0';
        mask = 1 << (i - 1);
        if ((xy.x & mask) != 0) {
            digit++;
        }
        if ((xy.y & mask) != 0) {
            digit++;
            digit++;
        }
        quadDigits.push(digit);
    }

		return this._url
				.replace('{culture}', this.supportedCultures[OSRM.Localization.current_language] || "en-US" )
				.replace('{subdomain}', subdomains[(xy.x + xy.y) % subdomains.length])
				.replace('{quadkey}', quadDigits.join(""));		
  },
  
  _updateAttribution: function() {
    if (this._map.attributionControl) {
      var metadata = this.metadata;
      var res = metadata.resourceSets[0].resources[0];
      var bounds = this._map.getBounds();
      var providers = res.imageryProviders, zoom = this._map.getZoom() + 1,
          copyrights = "", provider, i, ii, j, jj, bbox, coverage;
      for (i=0,ii=providers.length; i<ii; ++i) {
          provider = providers[i];
          for (j=0,jj=provider.coverageAreas.length; j<jj; ++j) {
              coverage = provider.coverageAreas[j];
              if (zoom <= coverage.zoomMax && zoom >= coverage.zoomMin && coverage.bbox.intersects(bounds)) {
                  copyrights += provider.attribution + " ";
                  j = jj;
              }
          }
      }
      this._map.attributionControl.removeAttribution(this.attribution);
      this._map.attributionControl._attributions = {};
      this._map.attributionControl._update();
      this.attribution = this.attributionTemplate
        .replace('{logo}', metadata.brandLogoUri)
        .replace('{copyrights}', copyrights);
      this._map.attributionControl.addAttribution(this.attribution);
    }
  }    
});

L.TileLayer.Bing._callbackId = 0;

L.TileLayer.Bing.processMetadata = function(metadata) {
  if (metadata.authenticationResultCode != 'ValidCredentials') {
    throw "Invalid Bing Maps API Key"
  }
  
  if (!metadata.resourceSets.length || !metadata.resourceSets[0].resources.length) {
    throw "No resources returned, perhaps " + this._mapType + " is an invalid map type?";
  }
  
  if (metadata.statusCode != 200) {
    throw "Bing Maps API request failed with status code " + metadata.statusCode;
  }
  
  this.metadata = metadata;
  var res = metadata.resourceSets[0].resources[0],
      providers = res.imageryProviders,
      i = 0,
      j,
      provider,
      bbox,
      script = document.getElementById(this._callbackId);
  
  for (; i<providers.length; i++) {
    provider = providers[i];
    for (j=0; j<provider.coverageAreas.length; j++) {
      bbox = provider.coverageAreas[j].bbox;
      provider.coverageAreas[j].bbox = new L.LatLngBounds(new L.LatLng(bbox[0],bbox[1],true),new L.LatLng(bbox[2],bbox[3], true));
    }
  }
  
  this._url = res.imageUrl;
  this.options.subdomains = [].concat(res.imageUrlSubdomains);
  script.parentNode.removeChild(script);
  window[this._callbackId] = undefined; // cannot delete from window in IE
  delete this._callbackId;
  this._onMetadataLoaded();
}