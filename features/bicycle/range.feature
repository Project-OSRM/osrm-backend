@bycicle
Feature: Getting the range limited by Distance

  Background:
    Given the profile "car"

  Scenario: Range
    Given the node map
      | a | b | c |

    And the ways
      | nodes |
      | ab    |
      | bc    |

    When I request range I should get
      |source | node  | pred |
      | a     |  b    |   c  |