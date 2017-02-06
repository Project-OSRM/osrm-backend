'use strict';

const fs = require('fs');
const util = require('util');
const d3 = require('d3-queue');
const classes = require('./data_classes');
const tableDiff = require('../lib/table_diff');
var child_process = require('child_process');
var path = require('path')


module.exports = function () {
  
  this.processWayTags = (tags) => {
    var args = []

    for (var key in tags) {
      args.push("\"" + key + "\"")
      args.push("\"" + tags[key] + "\"")
    }
    
    var oldDir = process.cwd()
    process.chdir('profiles')
    
    var lua = 'lua5.1'
    var profile = 'bicycle'
    var script  = './lib/testing.lua'
    var cmd = [lua,script,profile,args.join(' ')].join(' ')
    var output = child_process.execSync(cmd).toString()

    process.chdir(oldDir)

    return JSON.parse(output)
   };
};
