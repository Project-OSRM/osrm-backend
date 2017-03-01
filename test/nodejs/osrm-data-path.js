var path = require('path');

if (process.env.OSRM_DATA_PATH !== undefined) {
    exports.data_path = path.join(path.resolve(process.env.OSRM_DATA_PATH), "berlin_CH.osrm");
    console.log('Setting custom data path to ' + exports.data_path);
} else {
    exports.data_path = path.resolve(path.join(__dirname, "../data/berlin_CH.osrm"));
}
