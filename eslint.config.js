import { Linter } from 'eslint';

/** @type {Linter.Config} */
const config = [
  {
    files: ["**/*.js"],
    languageOptions: {
      ecmaVersion: 2024,
      sourceType: "module",
      globals: {
        es6: true,
        node: true,
      },
    },
    rules: {
      indent: ['error', 2],
      quotes: ['warn', 'single'],
      'linebreak-style': ['error', 'unix'],
      semi: ['error', 'always'],
      'no-console': ['warn'],
      'prefer-const': 'error',
      'no-var': 'error',
      'no-unused-vars': ['error', { argsIgnorePattern: '^_' }],
      'object-shorthand': 'error',
      'prefer-template': 'error',
      'prefer-arrow-callback': 'error',
    },
    ignores: ['node_modules/', 'build/', 'dist/', 'coverage/', 'features/support/fbresult_generated.js'],
  },
];

export default config;
