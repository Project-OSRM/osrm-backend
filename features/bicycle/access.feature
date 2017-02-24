@routing @bicycle @access
Feature: Bike - Access tags on ways
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Access tag hierarchy on ways
        Then routability should be
            | highway | access | vehicle | bicycle | bothw   |
            | primary |        |         |         | cycling |
            | primary | yes    |         |         | cycling |
            | primary | no     |         |         |         |
            | primary |        | yes     |         | cycling |
            | primary |        | no      |         |         |
            | primary | no     | yes     |         | cycling |
            | primary | yes    | no      |         |         |
            | primary |        |         | yes     | cycling |
            | primary |        |         | no      |         |
            | primary | no     |         | yes     | cycling |
            | primary | yes    |         | no      |         |
            | primary |        | no      | yes     | cycling |
            | primary |        | yes     | no      |         |

    @todo
    Scenario: Bike - Access tag in forward direction
        Then routability should be
            | highway | access:forward | vehicle:forward | bicycle:forward | forw    | backw |
            | primary |                |                 |                 | cycling |       |
            | primary | yes            |                 |                 | cycling |       |
            | primary | no             |                 |                 |         |       |
            | primary |                | yes             |                 | cycling |       |
            | primary |                | no              |                 |         |       |
            | primary | no             | yes             |                 | cycling |       |
            | primary | yes            | no              |                 |         |       |
            | primary |                |                 | yes             | cycling |       |
            | primary |                |                 | no              |         |       |
            | primary | no             |                 | yes             | cycling |       |
            | primary | yes            |                 | no              |         |       |
            | primary |                | no              | yes             | cycling |       |
            | primary |                | yes             | no              |         |       |
            | runway  |                |                 |                 | cycling |       |
            | runway  | yes            |                 |                 | cycling |       |
            | runway  | no             |                 |                 |         |       |
            | runway  |                | yes             |                 | cycling |       |
            | runway  |                | no              |                 |         |       |
            | runway  | no             | yes             |                 | cycling |       |
            | runway  | yes            | no              |                 |         |       |
            | runway  |                |                 | yes             | cycling |       |
            | runway  |                |                 | no              |         |       |
            | runway  | no             |                 | yes             | cycling |       |
            | runway  | yes            |                 | no              |         |       |
            | runway  |                | no              | yes             | cycling |       |
            | runway  |                | yes             | no              |         |       |

    @todo
    Scenario: Bike - Access tag in backward direction
        Then routability should be
            | highway | access:forward | vehicle:forward | bicycle:forward | forw | backw   |
            | primary |                |                 |                 |      | cycling |
            | primary | yes            |                 |                 |      | cycling |
            | primary | no             |                 |                 |      |         |
            | primary |                | yes             |                 |      | cycling |
            | primary |                | no              |                 |      |         |
            | primary | no             | yes             |                 |      | cycling |
            | primary | yes            | no              |                 |      |         |
            | primary |                |                 | yes             |      | cycling |
            | primary |                |                 | no              |      |         |
            | primary | no             |                 | yes             |      | cycling |
            | primary | yes            |                 | no              |      |         |
            | primary |                | no              | yes             |      | cycling |
            | primary |                | yes             | no              |      |         |
            | runway  |                |                 |                 |      | cycling |
            | runway  | yes            |                 |                 |      | cycling |
            | runway  | no             |                 |                 |      |         |
            | runway  |                | yes             |                 |      | cycling |
            | runway  |                | no              |                 |      |         |
            | runway  | no             | yes             |                 |      | cycling |
            | runway  | yes            | no              |                 |      |         |
            | runway  |                |                 | yes             |      | cycling |
            | runway  |                |                 | no              |      |         |
            | runway  | no             |                 | yes             |      | cycling |
            | runway  | yes            |                 | no              |      |         |
            | runway  |                | no              | yes             |      | cycling |
            | runway  |                | yes             | no              |      |         |

    Scenario: Bike - Overwriting implied acccess on ways
        Then routability should be
            | highway  | access | vehicle | bicycle | bothw   |
            | cycleway |        |         |         | cycling |
            | runway   |        |         |         |         |
            | cycleway | no     |         |         |         |
            | cycleway |        | no      |         |         |
            | cycleway |        |         | no      |         |
            | runway   | yes    |         |         | cycling |
            | runway   |        | yes     |         | cycling |
            | runway   |        |         | yes     | cycling |

    Scenario: Bike - Access tags on ways
        Then routability should be
            | access       | vehicle      | bicycle      | bothw   |
            |              |              |              | cycling |
            | yes          |              |              | cycling |
            | permissive   |              |              | cycling |
            | designated   |              |              | cycling |
            | some_tag     |              |              | cycling |
            | no           |              |              |         |
            | private      |              |              |         |
            | agricultural |              |              |         |
            | forestry     |              |              |         |
            | delivery     |              |              |         |
            |              | yes          |              | cycling |
            |              | permissive   |              | cycling |
            |              | designated   |              | cycling |
            |              | some_tag     |              | cycling |
            |              | no           |              |         |
            |              | private      |              |         |
            |              | agricultural |              |         |
            |              | forestry     |              |         |
            |              | delivery     |              |         |
            |              |              | yes          | cycling |
            |              |              | permissive   | cycling |
            |              |              | designated   | cycling |
            |              |              | some_tag     | cycling |
            |              |              | no           |         |
            |              |              | private      |         |
            |              |              | agricultural |         |
            |              |              | forestry     |         |
            |              |              | delivery     |         |

    Scenario: Bike - Access tags on both node and way
        Then routability should be
            | access   | node/access | bothw   |
            | yes      | yes         | cycling |
            | yes      | no          |         |
            | yes      | some_tag    | cycling |
            | no       | yes         |         |
            | no       | no          |         |
            | no       | some_tag    |         |
            | some_tag | yes         | cycling |
            | some_tag | no          |         |
            | some_tag | some_tag    | cycling |

    Scenario: Bike - Access combinations
        Then routability should be
            | highway     | access     | vehicle    | bicycle    | forw    | backw   |
            | runway      | private    |            | yes        | cycling | cycling |
            | footway     |            | no         | permissive | cycling | cycling |
            | motorway    |            |            | yes        | cycling |         |
            | track       | forestry   |            | permissive | cycling | cycling |
            | cycleway    | yes        | designated | no         |         |         |
            | primary     |            | yes        | private    |         |         |
            | residential | permissive |            | no         |         |         |

    Scenario: Bike - Ignore access tags for other modes
        Then routability should be
            | highway  | boat | motor_vehicle | moped | bothw   |
            | river    | yes  |               |       |         |
            | cycleway | no   |               |       | cycling |
            | runway   |      | yes           |       |         |
            | cycleway |      | no            |       | cycling |
            | runway   |      |               | yes   |         |
            | cycleway |      |               | no    | cycling |

    Scenario: Bike - Bridleways when access is explicit
        Then routability should be
            | highway   | horse      | foot | bicycle | bothw        |
            | bridleway |            |      | yes     | cycling      |
            | bridleway |            | yes  |         | pushing bike |
            | bridleway | designated |      |         |              |
            | bridleway |            |      |         |              |

    Scenario: Bike - Tram with oneway when access is implicit
        Then routability should be
            | highway     | railway | access | oneway | forw    | backw        |
            | residential | tram    |        | yes    | cycling | pushing bike |
            | service     | tram    | psv    | yes    | cycling | pushing bike |

    Scenario: Bike - Access combinations
        Then routability should be
            | highway    | access     | bothw   |
            | primary    | permissive | cycling |
            | steps      | permissive | cycling |
            | footway    | permissive | cycling |
            | garbagetag | permissive | cycling |
