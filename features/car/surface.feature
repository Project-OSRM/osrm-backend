@routing @car @surface
Feature: Car - Surfaces

    Background:
        Given the profile "car"

    Scenario: Car - Routability of tracktype tags
        Then routability should be
            | highway | tracktype | bothw |
            | trunk   | grade1    | x     |
            | trunk   | grade2    | x     |
            | trunk   | grade3    | x     |
            | trunk   | grade4    | x     |
            | trunk   | grade5    | x     |
            | trunk   | nonsense  | x     |

    Scenario: Car - Routability of smoothness tags
        Then routability should be
            | highway | smoothness    | bothw |
            | trunk   | excellent     | x     |
            | trunk   | good          | x     |
            | trunk   | intermediate  | x     |
            | trunk   | bad           | x     |
            | trunk   | very_bad      | x     |
            | trunk   | horrible      | x     |
            | trunk   | very_horrible | x     |
            | trunk   | impassable    |       |
            | trunk   | nonsense      | x     |

    Scenario: Car - Routability of surface tags
        Then routability should be
            | highway | surface  | bothw |
            | trunk   | asphalt  | x     |
            | trunk   | sett     | x     |
            | trunk   | gravel   | x     |
            | trunk   | nonsense | x     |

    Scenario: Car - Good surfaces should not grant access
        Then routability should be
            | highway  | access       | tracktype | smoothness | surface | forw | backw |
            | motorway |              |           |            |         | x    |       |
            | motorway | no           | grade1    | excellent  | asphalt |      |       |
            | motorway | private      | grade1    | excellent  | asphalt | x    |       |
            | motorway | agricultural | grade1    | excellent  | asphalt |      |       |
            | motorway | forestry     | grade1    | excellent  | asphalt |      |       |
            | motorway | emergency    | grade1    | excellent  | asphalt |      |       |
            | primary  |              |           |            |         | x    | x     |
            | primary  | private      | grade1    | excellent  | asphalt | x    | x     |
            | primary  | no           | grade1    | excellent  | asphalt |      |       |
            | primary  | agricultural | grade1    | excellent  | asphalt |      |       |
            | primary  | forestry     | grade1    | excellent  | asphalt |      |       |
            | primary  | emergency    | grade1    | excellent  | asphalt |      |       |

    Scenario: Car - Impassable surfaces should deny access
        Then routability should be
            | highway  | access | smoothness | forw | backw |
            | motorway |        | impassable |      |       |
            | motorway | yes    |            | x    |       |
            | motorway | yes    | impassable |      |       |
            | primary  |        | impassable |      |       |
            | primary  | yes    |            | x    | x     |
            | primary  | yes    | impassable |      |       |

    Scenario: Car - Surface should reduce speed
        Then routability should be
            | highway  | oneway | surface         | forw        | backw       |
            | motorway | no     |                 | 90 km/h     | 90 km/h     |
            | motorway | no     | asphalt         | 90 km/h     | 90 km/h +-1 |
            | motorway | no     | concrete        | 90 km/h +-1 | 90 km/h +-1 |
            | motorway | no     | concrete:plates | 90 km/h +-1 | 90 km/h +-1 |
            | motorway | no     | concrete:lanes  | 90 km/h +-1 | 90 km/h +-1 |
            | motorway | no     | paved           | 90 km/h +-1 | 90 km/h +-1 |
            | motorway | no     | cement          | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | compacted       | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | fine_gravel     | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | paving_stones   | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | metal           | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | bricks          | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | grass           | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | wood            | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | sett            | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | grass_paver     | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | gravel          | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | unpaved         | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | ground          | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | dirt            | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | pebblestone     | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | tartan          | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | cobblestone     | 30 km/h +-1 | 30 km/h +-1 |
            | motorway | no     | clay            | 30 km/h +-1 | 30 km/h +-1 |
            | motorway | no     | earth           | 20 km/h +-1 | 20 km/h +-1 |
            | motorway | no     | stone           | 20 km/h +-1 | 20 km/h +-1 |
            | motorway | no     | rocky           | 20 km/h +-1 | 20 km/h +-1 |
            | motorway | no     | sand            | 20 km/h +-1 | 20 km/h +-1 |
            | motorway | no     | mud             | 10 km/h +-1 | 10 km/h +-1 |

    Scenario: Car - Tracktypes should reduce speed
        Then routability should be
            | highway  | oneway | tracktype | forw        | backw       |
            | motorway | no     |           | 90 km/h     | 90 km/h     |
            | motorway | no     | grade1    | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | grade2    | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | grade3    | 30 km/h +-1 | 30 km/h +-1 |
            | motorway | no     | grade4    | 25 km/h +-1 | 25 km/h +-1 |
            | motorway | no     | grade5    | 20 km/h +-1 | 20 km/h +-1 |

    Scenario: Car - Smoothness should reduce speed
        Then routability should be
            | highway  | oneway | smoothness    | forw        | backw       |
            | motorway | no     |               | 90 km/h     | 90 km/h     |
            | motorway | no     | intermediate  | 80 km/h     | 80 km/h     |
            | motorway | no     | bad           | 40 km/h +-1 | 40 km/h +-1 |
            | motorway | no     | very_bad      | 20 km/h +-1 | 20 km/h +-1 |
            | motorway | no     | horrible      |  10 km/h +-1 |  10 km/h +-1 |
            | motorway | no     | very_horrible |  5 km/h +-1 |  5 km/h +-1 |

    Scenario: Car - Combination of surface tags should use lowest speed
        Then routability should be
            | highway  | oneway | tracktype | surface | smoothness    | bothw   |
            | motorway | no     |           |         |               | 90 km/h |
            | service  | no     | grade1    | asphalt | excellent     | 15 km/h |
            | motorway | no     | grade5    | asphalt | excellent     | 20 km/h |
            | motorway | no     | grade1    | mud     | excellent     | 10 km/h |
            | motorway | no     | grade1    | asphalt | very_horrible |  5 km/h |
            | service  | no     | grade5    | mud     | very_horrible |  5 km/h |

    Scenario: Car - Surfaces should not affect oneway direction
        Then routability should be
            | highway | oneway | tracktype | smoothness | surface  | forw | backw |
            | primary |        | grade1    | excellent  | asphalt  | x    | x     |
            | primary |        | grade5    | very_bad   | mud      | x    | x     |
            | primary |        | nonsense  | nonsense   | nonsense | x    | x     |
            | primary | no     | grade1    | excellent  | asphalt  | x    | x     |
            | primary | no     | grade5    | very_bad   | mud      | x    | x     |
            | primary | no     | nonsense  | nonsense   | nonsense | x    | x     |
            | primary | yes    | grade1    | excellent  | asphalt  | x    |       |
            | primary | yes    | grade5    | very_bad   | mud      | x    |       |
            | primary | yes    | nonsense  | nonsense   | nonsense | x    |       |
            | primary | -1     | grade1    | excellent  | asphalt  |      | x     |
            | primary | -1     | grade5    | very_bad   | mud      |      | x     |
            | primary | -1     | nonsense  | nonsense   | nonsense |      | x     |
