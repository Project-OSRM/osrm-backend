@datastore @options @help
Feature: osrm-datastore command line options: list

    Background:
        Given the profile "testbot"
        And the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been contracted

    Scenario: osrm-datastore - Help should be shown when no options are passed
        When I try to run "osrm-datastore --dataset-name test_dataset_42 {processed_file}"
        Then it should exit successfully
        When I try to run "osrm-datastore --list"
        Then it should exit successfully
        And stdout should contain "test_dataset_42/data"
