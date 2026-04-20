import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { osrm } from '../../features/support/fbresult_generated.js';

// Somewhere in Monaco
// http://www.openstreetmap.org/#map=18/43.73185/7.41772
export const three_test_coordinates = [[7.41337, 43.72956],
  [7.41546, 43.73077],
  [7.41862, 43.73216]];

export const two_test_coordinates = three_test_coordinates.slice(0, 2);

export const test_tile = {'at': [17059, 11948, 15], 'size': 159125};

const install_dir   = process.env.OSRM_NODEJS_INSTALL_DIR || '.';
const test_data_dir = process.env.OSRM_DATA_PATH || process.env.OSRM_TEST_DATA_DIR || 'test/data';

console.log(`# Setting installation path to: ${test_data_dir}`);
console.log(`# Setting custom data path to:  ${test_data_dir}`);

const data_path        = path.resolve(path.join(test_data_dir, 'ch', 'monaco.osrm'));
const mld_data_path    = path.resolve(path.join(test_data_dir, 'mld', 'monaco.osrm'));
const test_memory_path = path.resolve(path.join(test_data_dir, 'test_memory'));

const { default: OSRM, version } = await import(pathToFileURL(path.join(install_dir, 'lib', 'index.js')).href);

const FBResult = osrm.engine.api.fbresult.FBResult;

export { OSRM, FBResult, version, data_path, mld_data_path, test_memory_path };
