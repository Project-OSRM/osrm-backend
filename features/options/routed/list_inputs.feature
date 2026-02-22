@routed @options @list_inputs @no_datastore
Feature: osrm-routed command line options: list-inputs

    Background:
        Given the profile "testbot"

    Scenario: osrm-routed - List inputs
        When I run "osrm-routed --list-inputs"
        Then stderr should be empty
        And stdout should contain "required"
        And stdout should contain ".osrm."
        And it should exit successfully
