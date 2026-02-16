@datastore @options @help @isolated @no_datastore
Feature: osrm-datastore command line options

    Background:
        Given the profile "testbot"

    Scenario: osrm-datastore - Help should be shown when no options are passed
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been extracted
        And the data has been contracted

        When I try to run "osrm-datastore"
        Then stderr should contain "the argument for"
        And it should exit successfully

    Scenario: osrm-datastore - Updates should work
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been extracted
        And the data has been contracted

        When I run "osrm-datastore --spring-clean" with input "Y"
        And I try to run "osrm-datastore --dataset-name cucumber/updates_test {processed_file}"
        Then it should exit successfully
        When I try to run "osrm-datastore --list"
        Then it should exit successfully
        And stdout should contain "cucumber/updates_test/static"
        And stdout should contain "cucumber/updates_test/updatable"

    Scenario: osrm-datastore - Only metric update should work
        Given the node map
            """
            a b
            """
        And the ways
            | nodes |
            | ab    |
        And the data has been extracted
        And the data has been partitioned
        And the data has been customized

        When I run "osrm-datastore --spring-clean" with input "Y"
        And I try to run "osrm-datastore {processed_file} --dataset-name cucumber/only_metric_test"
        Then it should exit successfully

        Given the speed file
        """
        0,1,50
        """
        When I try to run "osrm-customize --segment-speed-file {speeds_file} {processed_file}"
        Then it should exit successfully
        When I try to run "osrm-datastore {processed_file} --dataset-name cucumber/only_metric_test --only-metric"
        Then it should exit successfully

    Scenario: osrm-datastore - Errors on invalid path
        When I try to run "osrm-datastore invalid_path.osrm"
        Then stderr should contain "[error] Config contains invalid file paths."
        And stderr should contain "Missing/Broken"
        And it should exit with an error
