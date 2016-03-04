var launchClasses = require('./launch_classes');

module.exports = function () {
    this._OSRMLoader = () => new (launchClasses._OSRMLoader.bind(launchClasses._OSRMLoader, this))();
};
