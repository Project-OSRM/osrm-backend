@partition @options @list_inputs
Feature: osrm-partition command line options: list-inputs

    Background:
        Given the profile "testbot"

    Scenario: osrm-partition - List inputs
        When I run "osrm-partition --list-inputs"
        Then stderr should be empty
        And stdout should contain "required"
        And stdout should contain ".osrm."
        And it should exit successfully
