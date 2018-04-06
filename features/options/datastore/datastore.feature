@datastore @options @help
Feature: osrm-datastore command line options

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
        And stdout should contain "test_dataset_42/static"
        And stdout should contain "test_dataset_42/updatable"

    Scenario: osrm-datastore - Only metric update should work
        Given the speed file
        """
        0,1,50
        """
        And the data has been extracted
        When I try to run "osrm-datastore {processed_file} --dataset-name cucumber/only_metric_test"
        Then it should exit successfully
        When I try to run "osrm-customize --segment-speed-file {speeds_file} {processed_file}"
        Then it should exit successfully
        When I try to run "osrm-datastore {processed_file} --dataset-name cucumber/only_metric_test --only-metric"
        Then it should exit successfully

    Scenario: osrm-datastore - Displaying help should work
        When I try to run "osrm-datastore {processed_file} --help"
        Then it should exit successfully

    Scenario: osrm-datastore - Errors on invalid path
        When I try to run "osrm-datastore invalid_path.osrm"
        Then stderr should contain "[error] Config contains invalid file paths."
        And stderr should contain "Missing/Broken"
        And it should exit with an error
