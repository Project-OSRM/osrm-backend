@contract @options @list_inputs
Feature: osrm-contract command line options: list-inputs

    Background:
        Given the profile "testbot"

    Scenario: osrm-contract - List inputs
        When I run "osrm-contract --list-inputs"
        Then stderr should be empty
        And stdout should contain "required"
        And stdout should contain ".osrm."
        And it should exit successfully
