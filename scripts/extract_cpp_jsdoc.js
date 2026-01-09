#!/usr/bin/env node
// Extract JSDoc comments from C++ files for documentation.js
// This replaces the removed --polyglot flag in documentation.js v14+

import { readFileSync } from 'fs';

if (process.argv.length < 3) {
    console.error('Usage: extract_cpp_jsdoc.js <file.cpp>');
    process.exit(1);
}

const input = readFileSync(process.argv[2], 'utf8');

// Extract /** ... */ comment blocks (non-greedy match)
const jsdocRegex = /\/\*\*[\s\S]*?\*\//g;
const blocks = input.match(jsdocRegex) || [];

console.log(blocks.join('\n\n'));
