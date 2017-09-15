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
            | primary        |          | 15 km/h    | 15 km/h    |       2.9 |        2.9 |
            | secondary      |          | 15 km/h    | 15 km/h    |       3.1 |        3.1 |
            | tertiary       |          | 15 km/h    | 15 km/h    |       3.3 |        3.3 |
            | primary_link   |          | 15 km/h    | 15 km/h    |       2.9 |        2.9 |
            | secondary_link |          | 15 km/h    | 15 km/h    |       3.1 |        3.1 |
            | tertiary_link  |          | 15 km/h    | 15 km/h    |       3.3 |        3.3 |
            | residential    |          | 15 km/h    | 15 km/h    |       4.2 |        4.2 |
            | cycleway       |          | 15 km/h    | 15 km/h    |       4.2 |        4.2 |
            | footway        |          | 6 km/h +-1 | 6 km/h +-1 |       1.4 |        1.4 |

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
            | footway        | track       | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | motorway       | lane        | 15 km/h |         |       4.2 |            |
            | primary        | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | secondary      | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary       | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | primary_link   | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | secondary_link | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary_link  | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | residential    | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | cycleway       | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | footway        | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | motorway       | shared_lane | 15 km/h |         |       4.2 |            |
            | primary        | shared_lane | 15 km/h | 15 km/h |       4.2 |        4.2 |

    Scenario: Bike - Apply no penalties to ways in direction of cycleways
        Then routability should be
            | highway        | cycleway:right | cycleway:left | forw        | backw       | forw_rate | backw_rate |
            | motorway       | track          |               | 15 km/h     |             |       4.2 |            |
            | primary        | track          |               | 15 km/h     | 15 km/h     |       4.2 |        2.9 |
            | secondary      | track          |               | 15 km/h     | 15 km/h     |       4.2 |        3.1 |
            | tertiary       | track          |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | primary_link   | track          |               | 15 km/h     | 15 km/h     |       4.2 |        2.9 |
            | secondary_link | track          |               | 15 km/h     | 15 km/h     |       4.2 |        3.1 |
            | tertiary_link  | track          |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | residential    | track          |               | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | cycleway       | track          |               | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        | track          |               | 15 km/h     | 6 km/h +-1  |       4.2 |        1.4 |
            | motorway       |                | track         |             | 15 km/h     |           |        4.2 |
            | primary        |                | track         | 15 km/h     | 15 km/h     |       2.9 |        4.2 |
            | secondary      |                | track         | 15 km/h     | 15 km/h     |       3.1 |        4.2 |
            | tertiary       |                | track         | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | primary_link   |                | track         | 15 km/h     | 15 km/h     |       2.9 |        4.2 |
            | secondary_link |                | track         | 15 km/h     | 15 km/h     |       3.1 |        4.2 |
            | tertiary_link  |                | track         | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | residential    |                | track         | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | cycleway       |                | track         | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        |                | track         | 6 km/h +-1  | 15 km/h     |       1.4 |        4.2 |
            | motorway       | lane           |               | 15 km/h     |             |       4.2 |            |
            | primary        | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        2.9 |
            | secondary      | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        3.1 |
            | tertiary       | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | primary_link   | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        2.9 |
            | secondary_link | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        3.1 |
            | tertiary_link  | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        3.3 |
            | residential    | lane           |               | 15 km/h +-1 | 15 km/h +-1 |       4.2 |        4.2 |
            | cycleway       | lane           |               | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        | lane           |               | 15 km/h     | 6 km/h +-1  |       4.2 |        1.4 |
            | motorway       |                | lane          |             | 15 km/h     |           |        4.2 |
            | primary        |                | lane          | 15 km/h     | 15 km/h     |       2.9 |        4.2 |
            | secondary      |                | lane          | 15 km/h +-1 | 15 km/h +-1 |       3.1 |        4.2 |
            | tertiary       |                | lane          | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | primary_link   |                | lane          | 15 km/h     | 15 km/h     |       2.9 |        4.2 |
            | secondary_link |                | lane          | 15 km/h     | 15 km/h     |       3.1 |        4.2 |
            | tertiary_link  |                | lane          | 15 km/h     | 15 km/h     |       3.3 |        4.2 |
            | residential    |                | lane          | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | cycleway       |                | lane          | 15 km/h     | 15 km/h     |       4.2 |        4.2 |
            | footway        |                | lane          | 6 km/h +-1  | 15 km/h     |       1.4 |        4.2 |
            | motorway       | shared_lane    |               | 15 km/h     |             |       4.2 |            |
            | primary        | shared_lane    |               | 15 km/h     | 15 km/h     |       4.2 |        2.9 |
            | motorway       |                | shared_lane   |             | 15 km/h     |           |        4.2 |
            | primary        |                | shared_lane   | 15 km/h     | 15 km/h     |       2.9 |        4.2 |


    Scenario: Bike - Don't apply penalties for all kind of cycleways
        Then routability should be
            | highway  | cycleway    | forw    | backw   | forw_rate | backw_rate |
            | tertiary | shared_lane | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary | lane        | 15 km/h | 15 km/h |       4.2 |        4.2 |
            | tertiary | opposite    | 15 km/h | 15 km/h |       3.3 |        4.2 |
