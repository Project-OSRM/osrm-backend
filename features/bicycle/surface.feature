@routing @surface @bicycle
Feature: Bike - Surfaces

    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - Slow surfaces
        Then routability should be
            | highway  | surface               | bothw   |
            | cycleway |                       | 48 s    |
            | cycleway | asphalt               | 47.9 s  |
            | cycleway | chipseal              | 48 s    |
            | cycleway | concrete              | 48 s    |
            | cycleway | concrete_lanes        | 48 s    |
            | cycleway | cobblestone:flattened | 72 s    |
            | cycleway | paving_stones         | 72 s    |
            | cycleway | wood                  | 72 s    |
            | cycleway | metal                 | 72 s    |
            | cycleway | compacted             | 72 s    |
            | cycleway | fine_gravel           | 72 s    |
            | cycleway | ground                | 72 s    |
            | cycleway | dirt                  | 90 s    |
            | cycleway | cobblestone           | 102.9 s |
            | cycleway | gravel                | 120 s   |
            | cycleway | pebblestone           | 120 s   |
            | cycleway | grass_paver           | 120 s   |
            | cycleway | dirt                  | 90 s    |
            | cycleway | earth                 | 120 s   |
            | cycleway | grass                 | 120 s   |
            | cycleway | mud                   | 240 s   |
            | cycleway | sand                  | 240 s   |
            | cycleway | woodchips             | 240 s   |
            | cycleway | sett                  | 80 s    |

    Scenario: Bicycle - Good surfaces on small paths
        Then routability should be
        | highway  | surface | bothw  |
        | cycleway |         | 48 s   |
        | path     |         | 55.3 s |
        | track    |         | 60 s   |
        | track    | asphalt | 60 s   |
        | path     | asphalt | 55.4 s |

    Scenario: Bicycle - Surfaces should not make unknown ways routable
        Then routability should be
        | highway  | surface | bothw |
        | cycleway |         | 48 s  |
        | nosense  |         |       |
        | nosense  | asphalt |       |

    Scenario: Bicycle - Surfaces should not increase speed when pushing bikes
      Given the node map
         """
         a b
         c d
         """

      And the ways
        | nodes | highway | oneway | surface |
        | ab    | primary | yes    | asphalt |
        | cd    | footway |        | asphalt |

      When I route I should get
        | from | to | route | modes                     | speed   |
        | a    | b  | ab,ab | cycling,cycling           | 15 km/h |
        | b    | a  | ab,ab | pushing bike,pushing bike | 4 km/h  |
        | c    | d  | cd,cd | pushing bike,pushing bike | 4 km/h  |
        | d    | c  | cd,cd | pushing bike,pushing bike | 4 km/h  |
