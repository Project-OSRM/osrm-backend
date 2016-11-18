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
            | private      |       |
            | agricultural |       |
            | forestry     |       |
            | psv          |       |
            | delivery     |       |
            | some_tag     | x     |


    Scenario: Car - Access tags on nodes
        Then routability should be
            | node/access  | bothw |
            | yes          | x     |
            | permissive   | x     |
            | designated   | x     |
            | no           |       |
            | private      |       |
            | agricultural |       |
            | forestry     |       |
            | psv          |       |
            | delivery     |       |
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
            | highway     | accesss      | vehicle    | motor_vehicle | motorcar    | bothw |
            | runway      | private      |            |               | permissive  | x     |
            | primary     | forestry     |            | yes           |             | x     |
            | cycleway    |              |            | designated    |             | x     |
            | residential |              | yes        | no            |             |       |
            | motorway    | yes          | permissive |               | private     |       |
            | trunk       | agricultural | designated | permissive    | no          |       |
            | pedestrian  |              |            |               |             |       |
            | pedestrian  |              |            |               | destination | x     |

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
    Scenario: Car - only designated HOV ways are ignored by default
        Then routability should be
            | highway | hov        | bothw |
            | primary | designated |       |
            | primary | yes        | x     |
            | primary | no         | x     |

    @hov
    Scenario: Car - a way with all lanes HOV-designated is inaccessible by default (similar to hov=designated)
        Then routability should be
            | highway | hov:lanes:forward      | hov:lanes:backward     | hov:lanes              | oneway | forw | backw |
            | primary | designated             | designated             |                        |        |      |       |
            | primary |                        | designated             |                        |        | x    |       |
            | primary | designated             |                        |                        |        |      | x     |
            | primary | designated\|designated | designated\|designated |                        |        |      |       |
            | primary | designated\|no         | designated\|no         |                        |        | x    | x     |
            | primary | yes\|no                | yes\|no                |                        |        | x    | x     |
            | primary |                        |                        |                        |        | x    | x     |
            | primary | designated             |                        |                        | -1     |      | x     |
            | primary |                        | designated             |                        | -1     |      |       |
            | primary |                        |                        | designated             | yes    |      |       |
            | primary |                        |                        | designated             | -1     |      |       |
            | primary |                        |                        | designated\|           | yes    | x    |       |
            | primary |                        |                        | designated\|           | -1     |      | x     |
            | primary |                        |                        | designated\|designated | yes    |      |       |
            | primary |                        |                        | designated\|designated | -1     |      |       |
            | primary |                        |                        | designated\|yes        | yes    | x    |       |
            | primary |                        |                        | designated\|no         | -1     |      | x     |

     Scenario: Car - these toll roads always work
        Then routability should be
            | highway | toll        | bothw |
            | primary | no          | x     |
            | primary | snowmobile  | x     |

     # To test this we need issue #2781
     @todo
     Scenario: Car - only toll=yes ways are ignored by default
        Then routability should be
            | highway | toll        | bothw |
            | primary | yes         |       |
