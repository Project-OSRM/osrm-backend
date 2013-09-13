@routing @surface @bicycle
Feature: Bike - Surfaces

    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - Slow surfaces
        Then routability should be
            | highway  | surface               | bothw |
            | cycleway |                       | 49s   |
            | cycleway | asphalt               | 49s   |
            | cycleway | cobblestone:flattened | 73s   |
            | cycleway | paving_stones         | 73s   |
            | cycleway | compacted             | 73s   |
            | cycleway | cobblestone           | 121s  |
            | cycleway | unpaved               | 121s  |
            | cycleway | fine_gravel           | 121s  |
            | cycleway | gravel                | 121s  |
            | cycleway | pebbelstone           | 121s  |
            | cycleway | dirt                  | 121s  |
            | cycleway | earth                 | 121s  |
            | cycleway | grass                 | 121s  |
            | cycleway | mud                   | 241s  |
            | cycleway | sand                  | 241s  |

    Scenario: Bicycle - Good surfaces on small paths
        Then routability should be
        | highway  | surface | bothw |
        | cycleway |         | 49s   |
        | path     |         | 61s   |
        | track    |         | 61s   |
        | track    | asphalt | 49s   |
        | path     | asphalt | 49s   |

    Scenario: Bicycle - Surfaces should not make unknown ways routable
        Then routability should be
        | highway  | surface | bothw |
        | cycleway |         | 49s   |
        | nosense  |         |       |
        | nosense  | asphalt |       |
