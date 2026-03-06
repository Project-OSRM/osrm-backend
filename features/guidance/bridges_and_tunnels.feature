@routing @car @bridge @tunnel @guidance
Feature: Car - Guidance - Bridges and Tunnels
    Background:
        Given the profile "car"
        And a grid size of 100 meters

    Scenario: Simple Bridge
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway | bridge | name               |
            | ab    | primary |        | Hauptstraße        |
            | bc    | primary | yes    | Hauptstraßenbrücke |
            | cd    | primary |        | Hauptstraße        |

        When I route I should get
            | from | to | route                   | turns | locations |
            | a | d | Hauptstraße,Hauptstraße | depart,arrive | a,d |

    Scenario: Bridge with Immediate Turn
        Given the node map
            """
                  d
            a   b c
                  e
            """

        And the ways
            | nodes | highway | bridge | name               |
            | ab    | primary |        | Hauptstraße        |
            | bc    | primary | yes    | Hauptstraßenbrücke |
            | dce   | primary |        | Nebenstraße        |

        When I route I should get
            | from | to | route                                      | turns | locations |
            | a | d | Hauptstraße,Nebenstraße,Nebenstraße | depart,turn left,arrive | a,e,d |
            | a | e | Hauptstraße,Nebenstraße,Nebenstraße | depart,turn right,arrive | a,e,e |
            | e | a | Nebenstraße,Hauptstraßenbrücke,Hauptstraße | depart,turn left,arrive | e,e,a |
            | d | a | Nebenstraße,Hauptstraßenbrücke,Hauptstraße | depart,turn right,arrive | d,e,a |

    Scenario: Bridge with Immediate Turn Front and Back
        Given the node map
            """
            f     d
            a   b c
            g     e
            """

        And the ways
            | nodes | highway | bridge | name               |
            | ab    | primary |        | Hauptstraße        |
            | bc    | primary | yes    | Hauptstraßenbrücke |
            | dce   | primary |        | Nebenstraße        |
            | gaf   | primary |        | Anderestraße       |

        When I route I should get
            | from | to | route                                                    | turns | locations |
            | f | d | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn left,turn left,arrive | f,e,e,d |
            | f | e | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn left,turn right,arrive | f,e,e,e |
            | g | d | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn right,turn left,arrive | g,e,e,d |
            | g | e | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn right,turn right,arrive | g,e,e,e |
            | e | f | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn left,turn right,arrive | e,e,e,f |
            | e | g | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn left,turn left,arrive | e,e,e,g |
            | d | f | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn right,turn right,arrive | d,e,e,f |
            | d | g | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn right,turn left,arrive | d,e,e,g |

    Scenario: Simple Tunnel
        Given the node map
            """
            a b c d
            """

        And the ways
            | nodes | highway | tunnel | name               |
            | ab    | primary |        | Hauptstraße        |
            | bc    | primary | yes    | Hauptstraßentunnel |
            | cd    | primary |        | Hauptstraße        |

        When I route I should get
            | from | to | route                   | turns | locations |
            | a | d | Hauptstraße,Hauptstraße | depart,arrive | a,d |

    Scenario: Tunnel with Immediate Turn
        Given the node map
            """
                  d
            a   b c
                  e
            """

        And the ways
            | nodes | highway | tunnel | name               |
            | ab    | primary |        | Hauptstraße        |
            | bc    | primary | yes    | Hauptstraßentunnel |
            | dce   | primary |        | Nebenstraße        |

        When I route I should get
            | from | to | route                                      | turns | locations |
            | a | d | Hauptstraße,Nebenstraße,Nebenstraße | depart,end of road left,arrive | a,e,d |
            | a | e | Hauptstraße,Nebenstraße,Nebenstraße | depart,end of road right,arrive | a,e,e |
            | e | a | Nebenstraße,Hauptstraßentunnel,Hauptstraße | depart,turn left,arrive | e,e,a |
            | d | a | Nebenstraße,Hauptstraßentunnel,Hauptstraße | depart,turn right,arrive | d,e,a |

    Scenario: Tunnel with Immediate Turn Front and Back
        Given the node map
            """
            f     d
            a   b c
            g     e
            """

        And the ways
            | nodes | highway | bridge | name               |
            | ab    | primary |        | Hauptstraße        |
            | bc    | primary | yes    | Hauptstraßentunnel |
            | dce   | primary |        | Nebenstraße        |
            | gaf   | primary |        | Anderestraße       |

        When I route I should get
            | from | to | route                                                    | turns | locations |
            | f | d | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn left,turn left,arrive | f,e,e,d |
            | f | e | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn left,turn right,arrive | f,e,e,e |
            | g | d | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn right,turn left,arrive | g,e,e,d |
            | g | e | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße | depart,turn right,turn right,arrive | g,e,e,e |
            | e | f | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn left,turn right,arrive | e,e,a,f |
            | e | g | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn left,turn left,arrive | e,e,a,g |
            | d | f | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn right,turn right,arrive | d,e,a,f |
            | d | g | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn right,turn left,arrive | d,e,a,g |
