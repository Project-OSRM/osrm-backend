@prepare @options @help
Feature: osrm-contract command line options: help

    Scenario: osrm-contract - Help should be shown when no options are passed
        When I try to run "osrm-contract"
        Then stderr should be empty
        And stdout should contain /osrm-contract(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--verbosity"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And stdout should contain "--core"
        And stdout should contain "--segment-speed-file"
        And it should exit with an error

    Scenario: osrm-contract - Help, short
        When I run "osrm-contract -h"
        Then stderr should be empty
        And stdout should contain /osrm-contract(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--verbosity"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And stdout should contain "--core"
        And stdout should contain "--segment-speed-file"
        And it should exit successfully

    Scenario: osrm-contract - Help, long
        When I run "osrm-contract --help"
        Then stderr should be empty
        And stdout should contain /osrm-contract(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--verbosity"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And stdout should contain "--core"
        And stdout should contain "--segment-speed-file"
        And it should exit successfully
