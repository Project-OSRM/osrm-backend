@datastore
Feature: Loading data through datastore
# Normally when launching osrm-routed, it will keep running as a server until it's shut down.
# For testing program options, the --trial option is used, which causes osrm-routed to quit
# immediately after initialization. This makes testing easier and faster.
# 
# The {base} part of the options to osrm-routed will be expanded to the actual base path of
# the preprocessed file.

    Background:
        Given the profile "testbot"

    Scenario: Load data with osrm-datastore - medium size
        Given the node map
            | a | b | c |
            | d | e | f |
        And the ways
            | nodes |
            | abc   |
            | def   |
            | ad    |
            | be    |
            | cf    |
        And the data has been prepared
        
        When I run "osrm-datastore --springclean"
        Then stderr should be empty
        And stdout should contain /^\[info\] spring-cleaning all shared memory regions/
        And it should exit with code 0
        
        When I run "osrm-datastore {base}.osrm"
        Then stderr should be empty
        And stdout should contain /^\[info\] all data loaded/
        And it should exit with code 0

        When I run "osrm-routed --sharedmemory --trial"
        Then stderr should be empty
        And stdout should contain /^\[info\] starting up engines/
        And stdout should contain /\d{1,2}\.\d{1,2}\.\d{1,2}/
        And stdout should contain /compiled at/
        And stdout should contain /^\[info\] loaded plugin: viaroute/
        And stdout should contain /^\[info\] trial run/
        And stdout should contain /^\[info\] shutdown completed/
        And it should exit with code 0

    Scenario: Load data with osrm-datastore - small size
        Given the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the data has been prepared

        When I run "osrm-datastore --springclean"
        Then stderr should be empty
        And stdout should contain /^\[info\] spring-cleaning all shared memory regions/
        And it should exit with code 0
        
        When I run "osrm-datastore {base}.osrm"
        Then stderr should be empty
        And stdout should contain /^\[info\] all data loaded/
        And it should exit with code 0

        When I run "osrm-routed --sharedmemory --trial"
        Then stderr should be empty
        And stdout should contain /^\[info\] starting up engines/
        And stdout should contain /\d{1,2}\.\d{1,2}\.\d{1,2}/
        And stdout should contain /compiled at/
        And stdout should contain /^\[info\] loaded plugin: viaroute/
        And stdout should contain /^\[info\] trial run/
        And stdout should contain /^\[info\] shutdown completed/
        And it should exit with code 0