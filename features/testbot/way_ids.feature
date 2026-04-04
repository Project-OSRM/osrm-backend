@routing @testbot
Feature: Route annotations - way ids

    Background:
        Given the profile "testbot"
        Given a grid size of 10 meters
        Given the node map
          """
          a--1-b--2-c--3-d
          """
        And the ways
          | nodes |
          | ab    |
          | bc    |
          | cd    |

    Scenario: Route annotations expose traversed OSM way ids per segment
        Given the query options
          | steps       | false   |
          | annotations | way_ids |

        When I route I should get
          | from | to | a:way_ids |
          | 1    | 3  | 5:6:7     |
          | 3    | 1  | 7:6:5     |
