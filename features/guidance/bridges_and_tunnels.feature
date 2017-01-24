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
            | from | to | route                   | turns         |
            | a    | d  | Hauptstraße,Hauptstraße | depart,arrive |

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
            | from | to | route                                      | turns                    |
            | a    | d  | Hauptstraße,Nebenstraße,Nebenstraße        | depart,turn left,arrive  |
            | a    | e  | Hauptstraße,Nebenstraße,Nebenstraße        | depart,turn right,arrive |
            | e    | a  | Nebenstraße,Hauptstraßenbrücke,Hauptstraße | depart,turn left,arrive  |
            | d    | a  | Nebenstraße,Hauptstraßenbrücke,Hauptstraße | depart,turn right,arrive |

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
            | from | to | route                                                    | turns                               |
            | f    | d  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn left,turn left,arrive   |
            | f    | e  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn left,turn right,arrive  |
            | g    | d  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn right,turn left,arrive  |
            | g    | e  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn right,turn right,arrive |
            | e    | f  | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn left,turn right,arrive  |
            | e    | g  | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn left,turn left,arrive   |
            | d    | f  | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn right,turn right,arrive |
            | d    | g  | Nebenstraße,Hauptstraßenbrücke,Anderestraße,Anderestraße | depart,turn right,turn left,arrive  |

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
            | from | to | route                   | turns         |
            | a    | d  | Hauptstraße,Hauptstraße | depart,arrive |

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
            | from | to | route                                      | turns                    |
            | a    | d  | Hauptstraße,Nebenstraße,Nebenstraße        | depart,turn left,arrive  |
            | a    | e  | Hauptstraße,Nebenstraße,Nebenstraße        | depart,turn right,arrive |
            | e    | a  | Nebenstraße,Hauptstraßentunnel,Hauptstraße | depart,turn left,arrive  |
            | d    | a  | Nebenstraße,Hauptstraßentunnel,Hauptstraße | depart,turn right,arrive |

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
            | from | to | route                                                    | turns                               |
            | f    | d  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn left,turn left,arrive   |
            | f    | e  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn left,turn right,arrive  |
            | g    | d  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn right,turn left,arrive  |
            | g    | e  | Anderestraße,Hauptstraße,Nebenstraße,Nebenstraße         | depart,turn right,turn right,arrive |
            | e    | f  | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn left,turn right,arrive  |
            | e    | g  | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn left,turn left,arrive   |
            | d    | f  | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn right,turn right,arrive |
            | d    | g  | Nebenstraße,Hauptstraßentunnel,Anderestraße,Anderestraße | depart,turn right,turn left,arrive  |

