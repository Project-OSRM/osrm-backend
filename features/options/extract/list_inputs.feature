@extract @options @list_inputs
Feature: osrm-extract command line options: list-inputs

    Background:
        Given the profile "testbot"

    Scenario: osrm-extract - List inputs
        When I run "osrm-extract --list-inputs"
        Then stderr should be empty
        And stdout should contain "required"
        And stdout should contain ".osrm."
        And it should exit successfully
