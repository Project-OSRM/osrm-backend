const { Linter } = require("eslint");

/** @type {Linter.Config} */
const config = [
    {
        rules: {
            indent: ["error", 2],
            quotes: ["warn", "single"],
            "linebreak-style": ["error", "unix"],
            semi: ["error", "always"],
            "no-console": ["warn"]
        },
        languageOptions: {
            globals: {
                es6: true,
                node: true
            }
        },
        ignores: [
            "node_modules/",
            "build/",
            "dist/",
            "coverage/"
        ]
    }
];

module.exports = config;
