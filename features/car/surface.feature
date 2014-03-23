@routing @car @surface
Feature: Car - Surface restrictions

    Background:
        Given the profile "car"

    Scenario: Car - Route over probably acceptable surfaces
        Then routability should be
            | tracktype | smoothness                  | bothw |
            | grade1    |                             | x     |
            | grade2    |                             | x     |
            | grade3    |                             | x     |
            | grade4    |                             | x     |
            | grade5    |                             | x     |
            |           | excellent                   | x     |
            |           | thin_rollers                | x     |
            |           | good                        | x     |
            |           | thin_wheels                 | x     |
            |           | intermediate                | x     |
            |           | wheels                      | x     |
            |           | bad                         | x     |
            |           | robust_wheels               | x     |
            |           | very_bad                    | x     |
            |           | high_clearance              | x     |

    Scenario: Car - Do not route over probably very rough surfaces
        Then routability should be
            | tracktype | smoothness                  | bothw |
            | grade6    |                             |       |
            | grade7    |                             |       |
            | grade8    |                             |       |
            |           | horrible                    |       |
            |           | off_road_wheels             |       |
            |           | very_horrible               |       |
            |           | specialized_off_road_wheels |       |
            |           | impassable                  |       |

    Scenario: Car - Reject values not in the closed set of tracktype and smoothness values
        Then routability should be
            | tracktype | smoothness                  | bothw |
            | foo       |                             |       |
            |           | foo                         |       |

    Scenario: Car - Forbidden access stays forbidden when using tracktype/smoothness
        Then routability should be
            | access  | tracktype | smoothness     | bothw |
            | no      | grade1    |                |       |
            | no      | grade5    |                |       |
            | no      |           | excellent      |       |
            | no      |           | thin_rollers   |       |
            | no      |           | very_bad       |       |
            | no      |           | high_clearance |       |
            | private | grade1    |                |       |
            | private | grade5    |                |       |
            | private |           | excellent      |       |
            | private |           | thin_rollers   |       |
            | private |           | very_bad       |       |
            | private |           | high_clearance |       |

    Scenario: Car - One-way remains one-way when using tracktype/smoothness
        Then routability should be
            | oneway | tracktype | smoothness     | forw | backw |
            | yes    | grade1    |                | x    |       |
            | yes    | grade5    |                | x    |       |
            | yes    |           | excellent      | x    |       |
            | yes    |           | thin_rollers   | x    |       |
            | yes    |           | very_bad       | x    |       |
            | yes    |           | high_clearance | x    |       |
            | -1     | grade1    |                |      | x     |
            | -1     | grade5    |                |      | x     |
            | -1     |           | excellent      |      | x     |
            | -1     |           | thin_rollers   |      | x     |
            | -1     |           | very_bad       |      | x     |
            | -1     |           | high_clearance |      | x     |

    Scenario: Car - Two-way remains two-way when using tracktype/smoothness
        Then routability should be
            | oneway | tracktype | smoothness     | bothw |
            | no     | grade1    |                | x     |
            | no     | grade5    |                | x     |
            | no     |           | excellent      | x     |
            | no     |           | thin_rollers   | x     |
            | no     |           | very_bad       | x     |
            | no     |           | high_clearance | x     |
            |        | grade1    |                | x     |
            |        | grade5    |                | x     |
            |        |           | excellent      | x     |
            |        |           | thin_rollers   | x     |
            |        |           | very_bad       | x     |
            |        |           | high_clearance | x     |

    Scenario: Car - Speed is not changed by the ideal values of tracktype/smoothness/surface
        Then routability should be
            | highway     | tracktype | smoothness   | surface | bothw    |
            | motorway    | grade1    | excellent    | asphalt | 9s ~23%  |
            | motorway    | grade1    | thin_rollers | asphalt | 9s ~23%  |
            | motorway    | grade1    |              |         | 9s ~23%  |
            | motorway    |           | excellent    |         | 9s ~23%  |
            | motorway    |           | thin_rollers |         | 9s ~23%  |
            | motorway    |           |              | asphalt | 9s ~23%  |
            | residential | grade1    | excellent    | asphalt | 30s ~10% |
            | residential | grade1    | thin_rollers | asphalt | 30s ~10% |
            | residential | grade1    |              |         | 30s ~10% |
            | residential |           | excellent    |         | 30s ~10% |
            | residential |           | thin_rollers |         | 30s ~10% |
            | residential |           |              | asphalt | 30s ~10% |

    Scenario: Car - Lower tracktype values decrease speed/preference
        Then routability should be
            | highway     | tracktype | bothw     |
            | motorway    | grade2    | 14s ~15%  |
            | motorway    | grade3    | 21s ~10%  |
            | motorway    | grade4    | 37s ~10%  |
            | motorway    | grade5    | 73s ~10%  |
            | residential | grade2    | 47s ~10%  |
            | residential | grade3    | 73s ~10%  |
            | residential | grade4    | 116s ~10% |
            | residential | grade5    | 181s ~10% |

    Scenario: Car - Lower smoothness values decrease speed/preference
        Then routability should be
            | highway     | smoothness     | bothw     |
            | motorway    | good           | 9s ~23%   |
            | motorway    | thin_wheels    | 9s ~23%   |
            | motorway    | intermediate   | 13s ~16%  |
            | motorway    | wheels         | 13s ~16%  |
            | motorway    | bad            | 49s ~10%  |
            | motorway    | robust_wheels  | 49s ~10%  |
            | motorway    | very_bad       | 181s ~10% |
            | motorway    | high_clearance | 181s ~10% |
            | residential | good           | 30s ~10%  |
            | residential | thin_wheels    | 30s ~10%  |
            | residential | intermediate   | 31s ~10%  |
            | residential | wheels         | 31s ~10%  |
            | residential | bad            | 121s ~10% |
            | residential | robust_wheels  | 121s ~10% |
            | residential | very_bad       | 481s ~10% |
            | residential | high_clearance | 481s ~10% |

    Scenario: Car - Maxspeed is subject to the preference factor associated with tracktype/smoothness/surface
        Then routability should be
            | highway     | tracktype | smoothness   | surface | maxspeed | bothw     |
            | motorway    | grade1    | excellent    | asphalt | 5        | 145s ~10% |
            | motorway    | grade1    | thin_rollers | asphalt | 5        | 145s ~10% |
            | motorway    | grade2    | intermediate | sett    | 5        | 230s ~10% |
            | motorway    | grade2    | wheels       | sett    | 5        | 230s ~10% |
            | motorway    | grade1    |              |         | 5        | 145s ~10% |
            | motorway    | grade2    |              |         | 5        | 230s ~10% |
            | motorway    |           | excellent    |         | 5        | 145s ~10% |
            | motorway    |           | thin_rollers |         | 5        | 145s ~10% |
            | motorway    |           | intermediate |         | 5        | 151s ~10% |
            | motorway    |           | wheels       |         | 5        | 151s ~10% |
            | motorway    |           |              | asphalt | 5        | 145s ~10% |
            | motorway    |           |              | sett    | 5        | 151s ~10% |
            | residential | grade1    | excellent    | asphalt | 5        | 145s ~10% |
            | residential | grade1    | thin_rollers | asphalt | 5        | 145s ~10% |
            | residential | grade2    | intermediate | sett    | 5        | 230s ~10% |
            | residential | grade2    | wheels       | sett    | 5        | 230s ~10% |
            | residential | grade1    |              |         | 5        | 145s ~10% |
            | residential | grade2    |              |         | 5        | 230s ~10% |
            | residential |           | excellent    |         | 5        | 145s ~10% |
            | residential |           | thin_rollers |         | 5        | 145s ~10% |
            | residential |           | intermediate |         | 5        | 151s ~10% |
            | residential |           | wheels       |         | 5        | 151s ~10% |
            | residential |           |              | asphalt | 5        | 145s ~10% |
            | residential |           |              | sett    | 5        | 151s ~10% |

    Scenario: Car - Directional maxspeed is subject to the preference factor associated with tracktype/smoothness/surface
        Then routability should be
            | highway     | tracktype | smoothness   | surface | maxspeed:forward | maxspeed:backward | forw      | backw     |
            | motorway    | grade1    | excellent    | asphalt | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | motorway    | grade1    | thin_rollers | asphalt | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | motorway    | grade2    | intermediate | sett    | 5                | 4                 | 115s ~10% | 144s ~10% |
            | motorway    | grade2    | wheels       | sett    | 5                | 4                 | 115s ~10% | 144s ~10% |
            | motorway    | grade1    |              |         | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | motorway    | grade2    |              |         | 5                | 4                 | 115s ~10% | 144s ~10% |
            | motorway    |           | excellent    |         | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | motorway    |           | thin_rollers |         | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | motorway    |           | intermediate |         | 5                | 4                 | 76s ~10%  | 95s ~10%  |
            | motorway    |           | wheels       |         | 5                | 4                 | 76s ~10%  | 95s ~10%  |
            | motorway    |           |              | asphalt | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | motorway    |           |              | sett    | 5                | 4                 | 76s ~10%  | 95s ~10%  |
            | residential | grade1    | excellent    | asphalt | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | residential | grade1    | thin_rollers | asphalt | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | residential | grade2    | intermediate | sett    | 5                | 4                 | 115s ~10% | 144s ~10% |
            | residential | grade2    | wheels       | sett    | 5                | 4                 | 115s ~10% | 144s ~10% |
            | residential | grade1    |              |         | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | residential | grade2    |              |         | 5                | 4                 | 115s ~10% | 144s ~10% |
            | residential |           | excellent    |         | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | residential |           | thin_rollers |         | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | residential |           | intermediate |         | 5                | 4                 | 76s ~10%  | 95s ~10%  |
            | residential |           | wheels       |         | 5                | 4                 | 76s ~10%  | 95s ~10%  |
            | residential |           |              | asphalt | 5                | 4                 | 73s ~10%  | 91s ~10%  |
            | residential |           |              | sett    | 5                | 4                 | 76s ~10%  | 95s ~10%  |

    Scenario: Car - Safe speed is more important than maxpeed in poor surfaces
        Then routability should be
            | highway     | maxspeed | tracktype | smoothness     | bothw     |
            | motorway    | 70       | grade5    |                | 73s ~10%  |
            | motorway    | 70       |           | very_bad       | 241s ~10% |
            | motorway    | 70       |           | high_clearance | 241s ~10% |
            | residential | 70       | grade5    |                | 73s ~10%  |
            | residential | 70       |           | very_bad       | 241s ~10% |
            | residential | 70       |           | high_clearance | 241s ~10% |
