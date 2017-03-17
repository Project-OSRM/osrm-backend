var path = require('path');

if (process.env.OSRM_DATA_PATH !== undefined) {
    exports.data_path = path.join(path.resolve(process.env.OSRM_DATA_PATH), "ch/berlin.osrm");
    exports.mld_data_path = path.join(path.resolve(process.env.OSRM_DATA_PATH), "mld/berlin.osrm");
    exports.corech_data_path = path.join(path.resolve(process.env.OSRM_DATA_PATH), "corech/berlin.osrm");
    console.log('Setting custom data path to ' + exports.data_path);
} else {
    exports.data_path = path.resolve(path.join(__dirname, "../data/ch/berlin.osrm"));
    exports.mld_data_path = path.resolve(path.join(__dirname, "../data/mld/berlin.osrm"));
    exports.corech_data_path = path.resolve(path.join(__dirname, "../data/corech/berlin.osrm"));
}
