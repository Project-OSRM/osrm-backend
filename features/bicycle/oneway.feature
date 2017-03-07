@routing @bicycle @oneway
Feature: Bike - Oneway streets
# Handle oneways streets, as defined at http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing
# Usually we can push bikes against oneways, but we use foot=no to prevent this in these tests

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Simple oneway
        Then routability should be
            | highway | foot | oneway | forw    | backw        |
            | primary | no   | yes    | cycling |              |
            | primary |      | yes    | cycling | pushing bike |

    Scenario: Simple reverse oneway
        Then routability should be
            | highway | foot | oneway | forw         | backw   |
            | primary | no   | -1     |              | cycling |
            | primary |      | -1     | pushing bike | cycling |

    Scenario: Bike - Around the Block
        Given the node map
            """
              a b
            f d c e
            """

        And the ways
            | nodes | oneway | foot |
            | ab    | yes    | no   |
            | bc    |        | no   |
            | cd    |        | no   |
            | da    |        | no   |
            | df    |        | no   |
            | ce    |        | no   |

        When I route I should get
            | from | to | route       |
            | a    | b  | ab,ab       |
            | b    | a  | bc,cd,da,da |

    Scenario: Bike - Handle various oneway tag values
        Then routability should be
            | foot | oneway   | forw    | backw   |
            | no   |          | cycling | cycling |
            | no   | nonsense | cycling | cycling |
            | no   | no       | cycling | cycling |
            | no   | false    | cycling | cycling |
            | no   | 0        | cycling | cycling |
            | no   | yes      | cycling |         |
            | no   | true     | cycling |         |
            | no   | 1        | cycling |         |
            | no   | -1       |         | cycling |

    Scenario: Bike - Implied oneways
        Then routability should be
            | highway       | foot | bicycle | junction   | forw    | backw   | #                     |
            |               | no   |         |            | cycling | cycling |                       |
            |               | no   |         | roundabout | cycling |         |                       |
            | motorway      | no   | yes     |            | cycling |         |                       |
            | motorway_link | no   | yes     |            | cycling | cycling | does not imply oneway |
            | motorway      | no   | yes     | roundabout | cycling |         |                       |
            | motorway_link | no   | yes     | roundabout | cycling |         |                       |

    Scenario: Bike - Overriding implied oneways
        Then routability should be
            | highway       | foot | junction   | oneway | forw    | backw   |
            | primary       | no   | roundabout | no     | cycling | cycling |
            | primary       | no   | roundabout | yes    | cycling |         |
            | motorway_link | no   |            | -1     |         |         |
            | trunk_link    | no   |            | -1     |         |         |
            | primary       | no   | roundabout | -1     |         | cycling |

    Scenario: Bike - Oneway:bicycle should override normal oneways tags
        Then routability should be
            | foot | oneway:bicycle | oneway | junction   | forw    | backw   |
            | no   | yes            |        |            | cycling |         |
            | no   | yes            | yes    |            | cycling |         |
            | no   | yes            | no     |            | cycling |         |
            | no   | yes            | -1     |            | cycling |         |
            | no   | yes            |        | roundabout | cycling |         |
            | no   | no             |        |            | cycling | cycling |
            | no   | no             | yes    |            | cycling | cycling |
            | no   | no             | no     |            | cycling | cycling |
            | no   | no             | -1     |            | cycling | cycling |
            | no   | no             |        | roundabout | cycling | cycling |
            | no   | -1             |        |            |         | cycling |
            | no   | -1             | yes    |            |         | cycling |
            | no   | -1             | no     |            |         | cycling |
            | no   | -1             | -1     |            |         | cycling |
            | no   | -1             |        | roundabout |         | cycling |

    Scenario: Bike - Contra flow
        Then routability should be
            | foot | oneway | cycleway       | forw    | backw   |
            | no   | yes    | opposite       | cycling | cycling |
            | no   | yes    | opposite_track | cycling | cycling |
            | no   | yes    | opposite_lane  | cycling | cycling |
            | no   | -1     | opposite       | cycling | cycling |
            | no   | -1     | opposite_track | cycling | cycling |
            | no   | -1     | opposite_lane  | cycling | cycling |
            | no   | no     | opposite       | cycling | cycling |
            | no   | no     | opposite_track | cycling | cycling |
            | no   | no     | opposite_lane  | cycling | cycling |

    Scenario: Bike - Should not be affected by car tags
        Then routability should be
            | foot | junction   | oneway | oneway:car | forw    | backw   |
            | no   |            | yes    | yes        | cycling |         |
            | no   |            | yes    | no         | cycling |         |
            | no   |            | yes    | -1         | cycling |         |
            | no   |            | no     | yes        | cycling | cycling |
            | no   |            | no     | no         | cycling | cycling |
            | no   |            | no     | -1         | cycling | cycling |
            | no   |            | -1     | yes        |         | cycling |
            | no   |            | -1     | no         |         | cycling |
            | no   |            | -1     | -1         |         | cycling |
            | no   | roundabout |        | yes        | cycling |         |
            | no   | roundabout |        | no         | cycling |         |
            | no   | roundabout |        | -1         | cycling |         |

    Scenario: Bike - Two consecutive oneways
        Given the node map
            """
            a b   c
            """

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |


        When I route I should get
            | from | to | route    |
            | a    | c  | ab,bc,bc |
