// Fuzzy matching utilities for approximate test result validation
import classes from './data_classes.js';

export default class Fuzzy {
  constructor(world) {
    this.world = world;
    this.FuzzyMatch = new classes.FuzzyMatch();
  }
}
