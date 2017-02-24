@routing @bicycle @access
Feature: Bike - Access tags on nodes
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Access tag hierarchy on nodes
        Then routability should be
            | node/access | node/vehicle | node/bicycle | node/highway | bothw   |
            |             |              |              |              | cycling |
            | yes         |              |              |              | cycling |
            | no          |              |              |              |         |
            |             | yes          |              |              | cycling |
            |             | no           |              |              |         |
            | no          | yes          |              |              | cycling |
            | yes         | no           |              |              |         |
            |             |              | yes          |              | cycling |
            |             |              | no           |              |         |
            |             |              | no           | crossing     | cycling |
            | no          |              | yes          |              | cycling |
            | yes         |              | no           |              |         |
            |             | no           | yes          |              | cycling |
            |             | yes          | no           |              |         |

    Scenario: Bike - Overwriting implied acccess on nodes doesn't overwrite way
        Then routability should be
            | highway  | node/access | node/vehicle | node/bicycle | bothw   |
            | cycleway |             |              |              | cycling |
            | runway   |             |              |              |         |
            | cycleway | no          |              |              |         |
            | cycleway |             | no           |              |         |
            | cycleway |             |              | no           |         |
            | runway   | yes         |              |              |         |
            | runway   |             | yes          |              |         |
            | runway   |             |              | yes          |         |

    Scenario: Bike - Access tags on nodes
        Then routability should be
            | node/access  | node/vehicle | node/bicycle | bothw   |
            |              |              |              | cycling |
            | yes          |              |              | cycling |
            | permissive   |              |              | cycling |
            | designated   |              |              | cycling |
            | some_tag     |              |              | cycling |
            | no           |              |              |         |
            | private      |              |              |         |
            | agricultural |              |              |         |
            | forestry     |              |              |         |
            | delivery     |              |              |         |
            |              | yes          |              | cycling |
            |              | permissive   |              | cycling |
            |              | designated   |              | cycling |
            |              | some_tag     |              | cycling |
            |              | no           |              |         |
            |              | private      |              |         |
            |              | agricultural |              |         |
            |              | forestry     |              |         |
            |              | delivery     |              |         |
            |              |              | yes          | cycling |
            |              |              | permissive   | cycling |
            |              |              | designated   | cycling |
            |              |              | some_tag     | cycling |
            |              |              | no           |         |
            |              |              | private      |         |
            |              |              | agricultural |         |
            |              |              | forestry     |         |
            |              |              | delivery     |         |
