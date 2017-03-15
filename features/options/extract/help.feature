@extract @options @help
Feature: osrm-extract command line options: help

    Background:
        Given the profile "testbot"

    Scenario: osrm-extract - Help should be shown when no options are passed
        When I run "osrm-extract"
        Then stderr should be empty
        And stdout should contain /osrm-extract(.exe)? <input.osm/.osm.bz2/.osm.pbf> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "Configuration:"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain "--small-component-size"
        And it should exit successfully

    Scenario: osrm-extract - Help, short
        When I run "osrm-extract -h"
        Then stderr should be empty
        And stdout should contain /osrm-extract(.exe)? <input.osm/.osm.bz2/.osm.pbf> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "Configuration:"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain "--small-component-size"
        And it should exit successfully

    Scenario: osrm-extract - Help, long
        When I run "osrm-extract --help"
        Then stderr should be empty
        And stdout should contain /osrm-extract(.exe)? <input.osm/.osm.bz2/.osm.pbf> \[options\]:/
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "Configuration:"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain "--small-component-size"
        And it should exit successfully
