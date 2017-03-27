@partition @options @help
Feature: osrm-partition command line options: help

    Scenario: osrm-partition - Help should be shown when no options are passed
        When I try to run "osrm-partition"
        Then stderr should be empty
        And stdout should contain /osrm-partition(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And stdout should contain "--balance"
        And stdout should contain "--boundary"
        And stdout should contain "--optimizing-cuts"
        And stdout should contain "--small-component-size"
        And stdout should contain "--max-cell-sizes"
        And it should exit with an error

    Scenario: osrm-partition - Help, short
        When I run "osrm-partition -h"
        Then stderr should be empty
        And stdout should contain /osrm-partition(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And stdout should contain "--balance"
        And stdout should contain "--boundary"
        And stdout should contain "--optimizing-cuts"
        And stdout should contain "--small-component-size"
        And stdout should contain "--max-cell-sizes"
        And it should exit successfully

    Scenario: osrm-partition - Help, long
        When I run "osrm-partition --help"
        Then stderr should be empty
        And stdout should contain /osrm-partition(.exe)? <input.osrm> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "Configuration:"
        And stdout should contain "--threads"
        And stdout should contain "--balance"
        And stdout should contain "--boundary"
        And stdout should contain "--optimizing-cuts"
        And stdout should contain "--small-component-size"
        And stdout should contain "--max-cell-sizes"
        And it should exit successfully
