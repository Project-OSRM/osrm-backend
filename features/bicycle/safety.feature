@routing @bicycle @safety
Feature: Bicycle - Adds penalties to unsafe roads

    Background:
        Given the profile file "bicycle" initialized with
        """
        profile.properties.weight_name = 'cyclability'
        """

    Scenario: Bike - Apply penalties to ways without cycleways
        Then routability should be
            | highway        | cycleway | forw       | backw      | forw_rate | backw_rate |
            | motorway       |          |            |            |           |            |
            | primary        |          | 15 km/h    | 15 km/h    |       2.1 |        2.1 |
            | secondary      |          | 15 km/h    | 15 km/h    |       2.7 |        2.7 |
            | tertiary       |          | 15 km/h    | 15 km/h    |       3.3 |        3.3 |
            | primary_link   |          | 15 km/h    | 15 km/h    |       2.1 |        2.1 |
            | secondary_link |          | 15 km/h    | 15 km/h    |       2.7 |        2.7 |
            | tertiary_link  |          | 15 km/h    | 15 km/h    |       3.3 |        3.3 |
            | residential    |          | 15 km/h    | 15 km/h    |       4.2 |        4.2 |
            | cycleway       |          | 15 km/h    | 15 km/h    |       4.2 |        4.2 |
            | footway        |          | 4 km/h +-1 | 4 km/h +-1 |       1.1 |        1.1 |

    Scenario: Bike - Apply no penalties to ways with cycleways
        Then routability should be
            | highway        | cycleway    | forw    | backw   | forw_rate | backw_rate |
            | motorway       | track       | 15 km/h |         |       4.2 |            |
            | primary        | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | secondary      | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary       | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | primary_link   | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | secondary_link | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary_link  | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | residential    | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | cycleway       | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | footway        | track       | 14 km/h | 14 km/h |       4.2 |        4.2 |
            | motorway       | lane        | 15 km/h |         |       4.2 |            |
            | primary        | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | secondary      | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary       | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | primary_link   | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | secondary_link | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary_link  | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | residential    | lane        | 14 km/h | 14 km/h |       4.2 |        4.2 |
            | cycleway       | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | footway        | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | motorway       | shared_lane | 15 km/h |         |       4.2 |            |
            | primary        | shared_lane | 15 km/h | 15 km/h |       4.2 |        4.2 |

    Scenario: Bike - Apply no penalties to ways in direction of cycleways
        Then routability should be
            | highway        | cycleway:right | cycleway:left | forw        | backw       | forw_rate | backw_rate |
            | motorway       | track          |               | 15 km/h     |             |       4.2 |            |
            | primary        | track          |               | 15 km/h     | 15 km/h     |       4.2 |        2.1 |
            | secondary      | track          |               | 15 km/h     | 15 km/h     |       4.2 |        2.7 |
            | tertiary       | track          |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | primary_link   | track          |               | 15 km/h     | 15 km/h     |       4.2 |        2.1 |
            | secondary_link | track          |               | 15 km/h     | 15 km/h     |       4.2 |        2.7 |
            | tertiary_link  | track          |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | residential    | track          |               | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | cycleway       | track          |               | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        | track          |               | 14 km/h     | 4 km/h +-1  |       4.2 |        1.1 |
            | motorway       |                | track         | 15 km/h     |             |       4.2 |            |
            | primary        |                | track         | 15 km/h     | 15 km/h     |       2.1 |        4.2 |
            | secondary      |                | track         | 15 km/h     | 15 km/h     |       2.7 |        4.2 |
            | tertiary       |                | track         | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | primary_link   |                | track         | 15 km/h     | 15 km/h     |       2.1 |        4.2 |
            | secondary_link |                | track         | 15 km/h     | 15 km/h     |       2.7 |        4.2 |
            | tertiary_link  |                | track         | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | residential    |                | track         | 14 km/h     | 14 km/h     |       4.2 |        4.2 |
            | cycleway       |                | track         | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        |                | track         | 4 km/h +-1  | 15 km/h     |       1.1 |        4.2 |
            | motorway       | lane           |               | 15 km/h     |             |       4.2 |            |
            | primary        | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        2.1 |
            | secondary      | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        2.7 |
            | tertiary       | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | primary_link   | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        2.1 |
            | secondary_link | lane           |               | 14 km/h     | 14 km/h     |       4.2 |        2.7 |
            | tertiary_link  | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | residential    | lane           |               | 15 km/h +-1 | 15 km/h +-1 |       4.2 |        4.2 |
            | cycleway       | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        | lane           |               | 15 km/h     | 4 km/h +-1  |       4.2 |        1.1 |
            | motorway       |                | lane          | 15 km/h     |             |       4.2 |            |
            | primary        |                | lane          | 15 km/h     | 15 km/h     |       2.1 |        4.2 |
            | secondary      |                | lane          | 15 km/h +-1 | 15 km/h +-1 |       2.7 |        4.2 |
            | tertiary       |                | lane          | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | primary_link   |                | lane          | 14 km/h     | 14 km/h     |       2.1 |        4.2 |
            | secondary_link |                | lane          | 15 km/h     | 15 km/h     |       2.7 |        4.2 |
            | tertiary_link  |                | lane          | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | residential    |                | lane          | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | cycleway       |                | lane          | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        |                | lane          | 4 km/h +-1  | 15 km/h     |       1.1 |        4.2 |
            | motorway       | shared_lane    |               | 15 km/h     |             |       4.2 |            |
            | primary        | shared_lane    |               | 15 km/h     | 15 km/h     |       4.2 |        2.1 |
            | motorway       |                | shared_lane   | 14 km/h     |             |       4.2 |            |
            | primary        |                | shared_lane   | 15 km/h     | 15 km/h     |       2.1 |        4.2 |


    Scenario: Bike - Don't apply penalties for all kind of cycleways
        Then routability should be
            | highway  | cycleway    | forw    | backw   | forw_rate | backw_rate |
            | tertiary | shared_lane | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary | opposite    | 15 km/h | 15 km/h |       3.3 |        3.3 |
