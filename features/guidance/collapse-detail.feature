@routing  @guidance @collapsing
Feature: Collapse

    Background:
        Given the profile "car"
        Given a grid size of 5 meters

    @reverse
    Scenario: Collapse U-Turn Triangle Intersection
        Given the node map
            """
            g---f---e---d
                \   /
                 \ /
            a-----b-----c
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | defg  | primary      | road | yes    |
            | fb    | primary_link |      | yes    |
            | be    | primary_link |      | yes    |

       When I route I should get
            | waypoints | route          | turns                        | locations |
            | a,g       | road,road,road | depart,continue uturn,arrive | a,b,g     |
            | d,c       | road,road,road | depart,continue uturn,arrive | d,f,c     |

    @reverse @traffic-signals
    Scenario: Collapse U-Turn Triangle Intersection
        Given the node map
            """
            g---f---j---e---d
                 \     /
                  h   i
                   \ /
            a-------b-------c
            """

        And the ways
            | nodes | highway      | name | oneway |
            | abc   | primary      | road | yes    |
            | dejfg | primary      | road | yes    |
            | fhb   | primary_link |      | yes    |
            | bie   | primary_link |      | yes    |

       And the nodes
            | node | highway         |
            | j    | traffic_signals |
            | h    | traffic_signals |
            | i    | traffic_signals |

       When I route I should get
            | waypoints | route          | turns                        | locations |
            | a,g       | road,road,road | depart,continue uturn,arrive | a,b,g     |
            | d,c       | road,road,road | depart,continue uturn,arrive | d,f,c     |

    Scenario: Forking before a turn (forky)
        Given the node map
            """
                      g
                      .
                      c
            a . . b .'
                    `d.
                     f e
            """
            # note: check collapse.feature for a similar test case where we do not
            # classify the situation as Sliproad and therefore keep the fork inst.

        And the ways
            | nodes | name  | oneway | highway   |
            | ab    | road  | yes    | primary   |
            | bd    | road  | yes    | primary   |
            | bc    | road  | yes    | primary   |
            | de    | road  | yes    | primary   |
            | fd    | cross | no     | secondary |
            | dc    | cross | no     | secondary |
            | cg    | cross | no     | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | dc     | d        | no_left_turn  |
            | restriction | bc       | dc     | c        | no_right_turn |

        When I route I should get
            | waypoints | route                 | turns                                          | locations |
            | a,g       | road,cross,cross      | depart,turn left,arrive                        | a,b,g     |
            | a,e       | road,road,road        | depart,continue slight right,arrive            | a,b,e     |
            # We should discuss whether the next item should be collapsed to depart,turn right,arrive.
            | a,f       | road,road,cross,cross | depart,continue slight right,turn right,arrive | a,b,d,f   |

    Scenario: Forking before a turn (forky), larger
        Given the node map
            """
                            g
                            .
                            c
						  .'|
					   .'   |
            a . . b .'      |
					  '	.   |
	                      ` d
    	                    f ' e
            """

        And the ways
            | nodes | name  | oneway | highway   |
            | ab    | road  | yes    | primary   |
            | bd    | road  | yes    | primary   |
            | bc    | road  | yes    | primary   |
            | de    | road  | yes    | primary   |
            | fd    | cross | no     | secondary |
            | dc    | cross | no     | secondary |
            | cg    | cross | no     | secondary |

        And the relations
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | bd       | dc     | d        | no_left_turn  |
            | restriction | bc       | dc     | c        | no_right_turn |

        When I route I should get
            | waypoints | route                 | turns                                          | locations |
            | a,g       | road,cross,cross      | depart,turn left,arrive                        | a,b,g     |
            | a,e       | road,road,road        | depart,continue slight right,arrive            | a,b,e     |
            # We should discuss whether the next item should be collapsed to depart,turn right,arrive.
            | a,f       | road,road,cross,cross | depart,continue slight right,turn right,arrive | a,b,d,f   |

    Scenario: Pulled Back Turn
        Given the node map
            """
                d
				|
            a-b-c
			  |
              e
            """

        And the ways
            | nodes | highway  | name  |
            | abc   | tertiary | road  |
            | cd    | tertiary | left  |
            | be    | tertiary | right |

        When I route I should get
            | waypoints | route            | turns                    |
            | a,d       | road,left,left   | depart,turn left,arrive  |
            | a,e       | road,right,right | depart,turn right,arrive |

    # https://www.openstreetmap.org/#map=18/53.89755/27.54306
    Scenario: Wide turn into invalid u-turn
        Given the node map
            """
                      g
                      |
                      |
                      f - - - - e
                      |
                      |
                      |
                h - - d
                      |
                      |
                      |
            a - - - - b - - - - c
            """

        And the ways
            | nodes | highway   | name |
            | abc   | secondary | road |
            | bdfg  | service   |      |
            | hd    | service   |      |
            | fe    | service   |      |

        When I route I should get
            | waypoints | route   | turns                               |
            | c,e       | road,,, | depart,turn right,turn right,arrive |
