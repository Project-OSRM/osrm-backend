import { readFileSync } from 'fs';
import { fileURLToPath } from 'url';
import path from 'path';
import { createRequire } from 'module';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const require = createRequire(import.meta.url);

const OSRM = require('./binding/node_osrm.node').OSRM;
const packageJson = JSON.parse(readFileSync(path.join(__dirname, '../package.json'), 'utf8'));

OSRM.version = packageJson.version;

export default OSRM;
