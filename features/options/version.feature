@routing @options
Feature: Command line options: version

    Background:
        Given the profile "testbot"
    
    Scenario Outline: Version
        When I run "osrm-routed <program_options>"
        Then stderr should be empty
        And stdout should contain 1 line
        And stdout should contain /v\d{1,2}\.\d{1,2}\.\d{1,2}/
        And it should exit with code 0

    Examples:
        | program_options |
        | -v              |
        | --version       |
