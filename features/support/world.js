'use strict';

const { setWorldConstructor } = require('@cucumber/cucumber');
const OSM = require('../lib/osm');
const OSRMLoader = require('../lib/osrm_loader');

// Import functionality from existing support modules
const envModule = require('./env');
const cacheModule = require('./cache');
const dataModule = require('./data');

class OSRMWorld {
  constructor() {
    // Initialize core components
    this.osrmLoader = null;
    this.OSMDB = null;
    
    // Bind methods from env.js
    envModule.call(this);
    
    // Bind methods from cache.js  
    cacheModule.call(this);
    
    // Bind methods from data.js
    dataModule.call(this);
  }
}

// Set the World constructor for Cucumber
setWorldConstructor(OSRMWorld);