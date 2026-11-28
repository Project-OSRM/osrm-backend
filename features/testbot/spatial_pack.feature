@routing @testbot
Feature: Querying spatially packed routing data

    Scenario: Snapping and annotated routing through reordered nodes
        Given the profile "testbot"
        And the origin -7,50
        And the node map
            """
            d   c 2 b   a
                  1
            """
        And the ways
            | nodes |
            | abcd  |
        And the dataset is spatially packed

        When I request nearest I should get
            | in | out | code | name | nodes |
            | 1  | 2   | Ok   | abcd | b,c   |

        And the query options
            | annotations | nodes |
        When I route I should get
            | from | to | route     | a:nodes |
            | a    | d  | abcd,abcd | abcd    |
            | d    | a  | abcd,abcd | dcba    |

        When I try to run "osrm-spatial-pack {processed_file} {processed_file}.repacked"
        Then it should exit with an error
        And stderr should contain "Query-only datasets cannot be modified"

    Scenario: The packer requires a prepared dataset
        When I try to run "osrm-spatial-pack --help"
        Then it should exit successfully
        And stdout should contain "Usage: osrm-spatial-pack"

        When I try to run "osrm-spatial-pack {processed_file}"
        Then it should exit with an error
        And stderr should contain "Usage: osrm-spatial-pack"

        When I try to run "osrm-spatial-pack {processed_file} {processed_file}.packed"
        Then it should exit with an error
        And stderr should contain "Expected a prepared CH or MLD routing dataset"
