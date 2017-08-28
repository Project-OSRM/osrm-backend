@contract @options @help
Feature: osrm-customize command line options: help

    Scenario: osrm-customize - Help should be shown when no options are passed
        When I try to run "osrm-customize"
        Then stderr should be empty
        And stdout should contain /osrm-customize(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--verbosity"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And it should exit with an error

    Scenario: osrm-customize - Help, short
        When I run "osrm-customize -h"
        Then stderr should be empty
        And stdout should contain /osrm-customize(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--verbosity"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And it should exit successfully

    Scenario: osrm-customize - Help, long
        When I run "osrm-customize --help"
        Then stderr should be empty
        And stdout should contain /osrm-customize(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--verbosity"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And it should exit successfully
