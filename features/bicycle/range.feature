@range
Feature: Getting the range limited by Distance

  Background:
    Given the profile "bicycle"

  Scenario: Range
    Given the node map
      | a |  | b |
      |   |  |   |
      |   |  | c |

    And the ways
      | nodes |
      | ab    |
      | ac    |

    When I request range I should get
      |source | node  | pred |
      | a     |  b    |   a  |
      | a     |  c    |   a  |
