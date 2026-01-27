#!/usr/bin/env node
// return the temp dir according to node

import os from 'node:os';

console.log(os.tmpdir());
