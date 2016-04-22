var exceptions = require('./exception_classes');

module.exports = function () {
    this.OSRMError = exceptions.OSRMError,

    this.FileError = (code, msg) => new (exceptions.FileError.bind(exceptions.FileError, this.PREPROCESS_LOG_FILE))(code, msg);

    this.LaunchError = (code, launchProcess, msg) => new (exceptions.LaunchError.bind(exceptions.LaunchError, this.ERROR_LOG_FILE))(code, launchProcess, msg);

    this.ExtractError = (code, msg) => new (exceptions.ExtractError.bind(exceptions.ExtractError, this.PREPROCESS_LOG_FILE))(code, msg);

    this.ContractError = (code, msg) => new (exceptions.ContractError.bind(exceptions.ContractError, this.PREPROCESS_LOG_FILE))(code, msg);

    this.RoutedError = (msg) => new (exceptions.RoutedError.bind(exceptions.RoutedError, this.OSRM_ROUTED_LOG_FILE))(msg);
};
