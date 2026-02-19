@datastore @options @list_inputs @no_datastore
Feature: osrm-datastore command line options: list-inputs

    Background:
        Given the profile "testbot"

    Scenario: osrm-datastore - List inputs
        When I run "osrm-datastore --list-inputs"
        Then stderr should be empty
        And stdout should contain "required"
        And stdout should contain ".osrm."
        And it should exit successfully
