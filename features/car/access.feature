@routing @car @access
Feature: Car - Restricted access
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "car"

    Scenario: Car - Access tag hierarchy on ways
        Then routability should be
            | access | vehicle | motor_vehicle | motorcar | bothw |
            |        |         |               |          | x     |
            | yes    |         |               |          | x     |
            | no     |         |               |          |       |
            |        | yes     |               |          | x     |
            |        | no      |               |          |       |
            | no     | yes     |               |          | x     |
            | yes    | no      |               |          |       |
            |        |         | yes           |          | x     |
            |        |         | no            |          |       |
            | no     |         | yes           |          | x     |
            | yes    |         | no            |          |       |
            |        | no      | yes           |          | x     |
            |        | yes     | no            |          |       |
            |        |         |               | yes      | x     |
            |        |         |               | no       |       |
            | no     |         |               | yes      | x     |
            | yes    |         |               | no       |       |
            |        | no      |               | yes      | x     |
            |        | yes     |               | no       |       |
            |        |         | no            | yes      | x     |
            |        |         | yes           | no       |       |

    Scenario: Car - Access tag hierarchy and forward/backward
        Then routability should be
            | access | access:forward | access:backward | motorcar | motorcar:forward | motorcar:backward | forw | backw |
            |        |                |                 |          |                  |                   | x    | x     |
            | yes    |                |                 |          |                  |                   | x    | x     |
            | yes    | no             |                 |          |                  |                   |      | x     |
            | yes    | yes            |                 | no       |                  |                   |      |       |
            | yes    | yes            |                 | yes      | no               |                   |      | x     |
            | yes    |                |                 |          |                  |                   | x    | x     |
            | yes    |                | no              |          |                  |                   | x    |       |
            | yes    |                | yes             | no       |                  |                   |      |       |
            | yes    |                | yes             | yes      |                  | no                | x    |       |
            | no     |                |                 |          |                  |                   |      |       |
            | no     | yes            |                 |          |                  |                   | x    |       |
            | no     | no             |                 | yes      |                  |                   | x    | x     |
            | no     | no             |                 | no       | yes              |                   | x    |       |
            | no     |                |                 |          |                  |                   |      |       |
            | no     |                | yes             |          |                  |                   |      | x     |
            | no     |                | no              | yes      |                  |                   | x    | x     |
            | no     |                | no              | no       |                  | yes               |      | x     |
            |        | no             |                 |          | no               |                   |      | x     |
            |        |                | no              |          |                  | no                | x    |       |
            |        | no             |                 |          |                  | no                |      |       |
            |        |                | no              | no       |                  |                   |      |       |
            |        | no             |                 |          | yes              |                   | x    | x     |
            |        |                | no              |          |                  | yes               | x    | x     |
            |        | yes            |                 |          | no               |                   |      | x     |
            |        |                | yes             |          |                  | no                | x    |       |

    Scenario: Car - Access tag hierarchy on nodes
        Then routability should be
            | node/access | node/vehicle | node/motor_vehicle | node/motorcar | bothw |
            |             |              |                    |               | x     |
            | yes         |              |                    |               | x     |
            | no          |              |                    |               |       |
            |             | yes          |                    |               | x     |
            |             | no           |                    |               |       |
            | no          | yes          |                    |               | x     |
            | yes         | no           |                    |               |       |
            |             |              | yes                |               | x     |
            |             |              | no                 |               |       |
            | no          |              | yes                |               | x     |
            | yes         |              | no                 |               |       |
            |             | no           | yes                |               | x     |
            |             | yes          | no                 |               |       |
            |             |              |                    | yes           | x     |
            |             |              |                    | no            |       |
            | no          |              |                    | yes           | x     |
            | yes         |              |                    | no            |       |
            |             | no           |                    | yes           | x     |
            |             | yes          |                    | no            |       |
            |             |              | no                 | yes           | x     |
            |             |              | yes                | no            |       |

    Scenario: Car - Overwriting implied acccess on ways
        Then routability should be
            | highway | access | vehicle | motor_vehicle | motorcar | bothw |
            | primary |        |         |               |          | x     |
            | runway  |        |         |               |          |       |
            | primary | no     |         |               |          |       |
            | primary |        | no      |               |          |       |
            | primary |        |         | no            |          |       |
            | primary |        |         |               | no       |       |
            | runway  | yes    |         |               |          | x     |
            | runway  |        | yes     |               |          | x     |
            | runway  |        |         | yes           |          | x     |
            | runway  |        |         |               | yes      | x     |

    Scenario: Car - Overwriting implied acccess on nodes
        Then routability should be
            | highway | node/access | node/vehicle | node/motor_vehicle | node/motorcar | bothw |
            | primary |             |              |                    |               | x     |
            | runway  |             |              |                    |               |       |
            | primary | no          |              |                    |               |       |
            | primary |             | no           |                    |               |       |
            | primary |             |              | no                 |               |       |
            | primary |             |              |                    | no            |       |
            | runway  | yes         |              |                    |               |       |
            | runway  |             | yes          |                    |               |       |
            | runway  |             |              | yes                |               |       |
            | runway  |             |              |                    | yes           |       |

    Scenario: Car - Access tags on ways
        Then routability should be
            | access       | bothw |
            | yes          | x     |
            | permissive   | x     |
            | designated   | x     |
            | no           |       |
            | private      | x     |
            | agricultural |       |
            | forestry     |       |
            | psv          |       |
            | delivery     | x     |
            | some_tag     | x     |
            | destination  | x     |


    Scenario: Car - Access tags on nodes
        Then routability should be
            | node/access  | bothw |
            | yes          | x     |
            | permissive   | x     |
            | designated   | x     |
            | no           |       |
            | private      | x     |
            | agricultural |       |
            | forestry     |       |
            | psv          |       |
            | delivery     | x     |
            | some_tag     | x     |

    Scenario: Car - Access tags on both node and way
        Then routability should be
            | access   | node/access | bothw |
            | yes      | yes         | x     |
            | yes      | no          |       |
            | yes      | some_tag    | x     |
            | no       | yes         |       |
            | no       | no          |       |
            | no       | some_tag    |       |
            | some_tag | yes         | x     |
            | some_tag | no          |       |
            | some_tag | some_tag    | x     |

    Scenario: Car - Access combinations
        Then routability should be
            | highway     | access       | vehicle    | motor_vehicle | motorcar    | forw | backw | # |
            | runway      | private      |            |               | permissive  | x    | x     |   |
            | primary     | forestry     |            | yes           |             | x    | x     |   |
            | cycleway    |              |            | designated    |             | x    | x     |   |
            | residential |              | yes        | no            |             |      |       |   |
            | motorway    | yes          | permissive |               | private     | x    |       | implied oneway  |
            | trunk       | agricultural | designated | permissive    | no          |      |       |   |
            | pedestrian  |              |            |               |             |      |       |   |
            | pedestrian  |              |            |               | destination | x    | x     |   |

    Scenario: Car - Ignore access tags for other modes
        Then routability should be
            | highway | foot | bicycle | psv | motorhome | bothw |
            | runway  | yes  |         |     |           |       |
            | primary | no   |         |     |           | x     |
            | runway  |      | yes     |     |           |       |
            | primary |      | no      |     |           | x     |
            | runway  |      |         | yes |           |       |
            | primary |      |         | no  |           | x     |
            | runway  |      |         |     | yes       |       |
            | primary |      |         |     | no        | x     |

    @hov
    Scenario: Car - designated HOV ways are rated low
        Then routability should be
            | highway | hov        | bothw | forw_rate  | backw_rate  |
            | primary | designated | x     | 18         | 18          |
            | primary | yes        | x     | 18         | 18          |
            | primary | no         | x     | 18         | 18          |

    # Models:
    # https://www.openstreetmap.org/way/124891268
    # https://www.openstreetmap.org/way/237173472
    @hov
    Scenario: Car - I-66 use HOV-only roads with heavy penalty
        Then routability should be
            | highway  | hov         | hov:lanes                          | lanes | access     | oneway | forw | backw | forw_rate  |
            | motorway | designated  | designated\|designated\|designated | 3     | hov        | yes    | x    |       | 25         |
            | motorway | lane        |                                    | 3     | designated | yes    | x    |       | 25         |

    @hov
    Scenario: Car - a way with all lanes HOV-designated is highly penalized by default (similar to hov=designated)
        Then routability should be
            | highway | hov:lanes:forward      | hov:lanes:backward     | hov:lanes              | oneway | forw | backw | forw_rate | backw_rate |
            | primary | designated             | designated             |                        |        | x    | x     | 18        | 18         |
            | primary |                        | designated             |                        |        | x    | x     | 18        | 18         |
            | primary | designated             |                        |                        |        | x    | x     | 18        | 18         |
            | primary | designated\|designated | designated\|designated |                        |        | x    | x     | 18        | 18         |
            | primary | designated\|no         | designated\|no         |                        |        | x    | x     | 18        | 18         |
            | primary | yes\|no                | yes\|no                |                        |        | x    | x     | 18        | 18         |
            | primary |                        |                        |                        |        | x    | x     | 18        | 18         |
            | primary | designated             |                        |                        | -1     |      | x     |           | 18         |
            | primary |                        | designated             |                        | -1     |      | x     |           | 18         |
            | primary |                        |                        | designated             | yes    | x    |       | 18        |            |
            | primary |                        |                        | designated             | -1     |      | x     |           | 18         |
            | primary |                        |                        | designated\|           | yes    | x    |       | 18        |            |
            | primary |                        |                        | designated\|           | -1     |      | x     |           | 18         |
            | primary |                        |                        | designated\|designated | yes    | x    |       | 18        |            |
            | primary |                        |                        | designated\|designated | -1     |      | x     |           | 18         |
            | primary |                        |                        | designated\|yes        | yes    | x    |       | 18        |            |
            | primary |                        |                        | designated\|no         | -1     |      | x     |           | 18         |

     Scenario: Car - these toll roads always work
        Then routability should be
            | highway | toll        | bothw |
            | primary | no          | x     |
            | primary | snowmobile  | x     |

     Scenario: Car - toll=yes ways are enabled by default
        Then routability should be
            | highway | toll        | bothw |
            | primary | yes         | x     |

    Scenario: Car - directional access tags
        Then routability should be
            | highway | access | access:forward | access:backward | forw | backw |
            | primary | yes    | yes            | yes             | x    | x     |
            | primary | yes    |                | no              | x    |       |
            | primary | yes    | no             |                 |      | x     |
            | primary | yes    | no             | no              |      |       |
            | primary | no     | no             | no              |      |       |
            | primary | no     |                | yes             |      | x     |
            | primary | no     | yes            |                 | x    |       |
            | primary | no     | yes            | yes             | x    | x     |


     Scenario: Car - barrier=gate should be routed over unless explicitely forbidden
        Then routability should be
            | node/barrier | access     | bothw |
            | gate         |            | x     |
            | gate         | no         |       |
            | gate         | yes        | x     |
            | gate         | permissive | x     |
            | gate         | designated | x     |
            | gate         | private    | x     |
            | gate         | garbagetag | x     |

    Scenario: Car - a way with conditional access
        Then routability should be
            | highway    | vehicle:forward | vehicle:backward:conditional | forw | backw |
            | pedestrian | yes             | delivery @ (20:00-11:00)     | x    |       |

    Scenario: Car - a way with a list of tags
        Then routability should be
            | highway | motor_vehicle            | motor_vehicle:forward | motor_vehicle:backward | forw | backw |
            | footway |                          |                       | destination            |      | x     |
            | track   | destination;agricultural | destination           |                        | x    | x     |

    Scenario: Car - Don't route over steps even if marked as accessible
        Then routability should be
            | highway | access | forw | backw |
            | steps   | yes    |      |       |
            | steps   | no     |      |       |
            | primary |        |  x   |   x   |

    Scenario: Car - Access combinations
        Then routability should be
            | highway    | access     | bothw |
            | primary    | permissive | x     |
            | steps      | permissive |       |
            | footway    | permissive | x     |
            | garbagetag | permissive | x     |
