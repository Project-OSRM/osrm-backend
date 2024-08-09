@routing @countryfoot @access
Feature: Countryfoot - Access tags on nodes
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "countryfoot"

    Scenario: Countryfoot - Access tag hierarchy on nodes
        Then routability should be
            | node/access | node/foot | bothw |
            |             |           | x     |
            |             | yes       | x     |
            |             | no        |       |
            | yes         |           | x     |
            | yes         | yes       | x     |
            | yes         | no        |       |
            | no          |           |       |
            | no          | yes       | x     |
            | no          | no        |       |

    Scenario: Countryfoot - Overwriting implied acccess on nodes doesn't overwrite way
        Then routability should be
            | highway  | node/access | node/foot | bothw |
            | footway  |             |           | x     |
            | footway  | no          |           |       |
            | footway  |             | no        |       |
            | motorway |             |           |       |
            | motorway | yes         |           |       |
            | motorway |             | yes       |       |

    Scenario: Countryfoot - Access tags on nodes
        Then routability should be
            | node/access  | node/foot    | bothw |
            |              |              | x     |
            | yes          |              | x     |
            | permissive   |              | x     |
            | designated   |              | x     |
            | some_tag     |              | x     |
            | no           |              |       |
            | private      |              |       |
            | agricultural |              |       |
            | forestry     |              |       |
            | delivery     |              |       |
            | no           | yes          | x     |
            | no           | permissive   | x     |
            | no           | designated   | x     |
            | no           | some_tag     | x     |
            | yes          | no           |       |
            | yes          | private      |       |
            | yes          | agricultural |       |
            | yes          | forestry     |       |
            | yes          | delivery     |       |
