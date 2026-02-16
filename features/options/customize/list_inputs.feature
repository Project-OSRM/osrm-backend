@prepare @options @list_inputs
Feature: osrm-customize command line options: list-inputs

    Background:
        Given the profile "testbot"

    Scenario: osrm-customize - List inputs
        When I run "osrm-customize --list-inputs"
        Then stderr should be empty
        And stdout should contain "required"
        And stdout should contain ".osrm."
        And it should exit successfully
