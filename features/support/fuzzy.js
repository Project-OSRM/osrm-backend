// Fuzzy matching utilities for approximate test result validation
var classes = require('./data_classes');

module.exports = function() {
  this.FuzzyMatch = new classes.FuzzyMatch();
};
