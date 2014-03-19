@routing @options
Feature: Command line options

    Background:
        Given the profile "testbot"

    Scenario: No options
        When I run "osrm-routed"
        Then it should exit with code 0
        And stderr should be empty
        And stdout should contain "osrm-routed <base.osrm> [<options>]:"
        And stdout should contain "Options:"
        And stdout should contain "Configuration:"

    Scenario: Help
        When I run "osrm-routed --help"
        Then it should exit with code 0
        And stderr should be empty
        And stdout should contain "osrm-routed <base.osrm> [<options>]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--hsgrdata arg"
        And stdout should contain "--nodesdata arg"
        And stdout should contain "--edgesdata arg"
        And stdout should contain "--ramindex arg"
        And stdout should contain "--fileindex arg"
        And stdout should contain "--namesdata arg"
        And stdout should contain "--timestamp arg"
        And stdout should contain "--ip"
        And stdout should contain "--port"
        And stdout should contain "--threads"
        And stdout should contain "--sharedmemory"
    
    @todo
    Scenario: Non-existing option
        When I run "osrm-routed --fly-me-to-the-moon"
        Then it should exit with code 255
        Then stdout should be empty
        And stderr should contain "unrecognised option '--fly-me-to-the-moon'"
    
    @todo
    Scenario: Missing file
        When I run "osrm-routed overtherainbow.osrm"
        Then it should exit with code 255
        Then stdout should be empty
        And stderr should contain "missing"