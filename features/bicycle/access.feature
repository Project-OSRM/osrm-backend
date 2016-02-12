@routing @bicycle @access
Feature: Bike - Access tags on ways
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Access tag hierachy on ways
        Then routability should be
            | highway | foot | access | vehicle | bicycle | bothw |
            |         | no   |        |         |         | x     |
            |         | no   | yes    |         |         | x     |
            |         | no   | no     |         |         |       |
            |         | no   |        | yes     |         | x     |
            |         | no   |        | no      |         |       |
            |         | no   | no     | yes     |         | x     |
            |         | no   | yes    | no      |         |       |
            |         | no   |        |         | yes     | x     |
            |         | no   |        |         | no      |       |
            |         | no   | no     |         | yes     | x     |
            |         | no   | yes    |         | no      |       |
            |         | no   |        | no      | yes     | x     |
            |         | no   |        | yes     | no      |       |
            | runway  | no   |        |         |         |       |
            | runway  | no   | yes    |         |         | x     |
            | runway  | no   | no     |         |         |       |
            | runway  | no   |        | yes     |         | x     |
            | runway  | no   |        | no      |         |       |
            | runway  | no   | no     | yes     |         | x     |
            | runway  | no   | yes    | no      |         |       |
            | runway  | no   |        |         | yes     | x     |
            | runway  | no   |        |         | no      |       |
            | runway  | no   | no     |         | yes     | x     |
            | runway  | no   | yes    |         | no      |       |
            | runway  | no   |        | no      | yes     | x     |
            | runway  | no   |        | yes     | no      |       |

    @todo
    Scenario: Bike - Access tag in forward direction
        Then routability should be
            | highway | foot | access:forward | vehicle:forward | bicycle:forward | forw | backw |
            |         | no   |                |                 |                 | x    |       |
            |         | no   | yes            |                 |                 | x    |       |
            |         | no   | no             |                 |                 |      |       |
            |         | no   |                | yes             |                 | x    |       |
            |         | no   |                | no              |                 |      |       |
            |         | no   | no             | yes             |                 | x    |       |
            |         | no   | yes            | no              |                 |      |       |
            |         | no   |                |                 | yes             | x    |       |
            |         | no   |                |                 | no              |      |       |
            |         | no   | no             |                 | yes             | x    |       |
            |         | no   | yes            |                 | no              |      |       |
            |         | no   |                | no              | yes             | x    |       |
            |         | no   |                | yes             | no              |      |       |
            | runway  | no   |                |                 |                 | x    |       |
            | runway  | no   | yes            |                 |                 | x    |       |
            | runway  | no   | no             |                 |                 |      |       |
            | runway  | no   |                | yes             |                 | x    |       |
            | runway  | no   |                | no              |                 |      |       |
            | runway  | no   | no             | yes             |                 | x    |       |
            | runway  | no   | yes            | no              |                 |      |       |
            | runway  | no   |                |                 | yes             | x    |       |
            | runway  | no   |                |                 | no              |      |       |
            | runway  | no   | no             |                 | yes             | x    |       |
            | runway  | no   | yes            |                 | no              |      |       |
            | runway  | no   |                | no              | yes             | x    |       |
            | runway  | no   |                | yes             | no              |      |       |

    @todo
    Scenario: Bike - Access tag in backward direction
        Then routability should be
            | highway | foot  | access:forward | vehicle:forward | bicycle:forward | forw | backw |
            |         | no    |                |                 |                 |      | x     |
            |         | no    | yes            |                 |                 |      | x     |
            |         | no    | no             |                 |                 |      |       |
            |         | no    |                | yes             |                 |      | x     |
            |         | no    |                | no              |                 |      |       |
            |         | no    | no             | yes             |                 |      | x     |
            |         | no    | yes            | no              |                 |      |       |
            |         | no    |                |                 | yes             |      | x     |
            |         | no    |                |                 | no              |      |       |
            |         | no    | no             |                 | yes             |      | x     |
            |         | no    | yes            |                 | no              |      |       |
            |         | no    |                | no              | yes             |      | x     |
            |         | no    |                | yes             | no              |      |       |
            | runway  | no    |                |                 |                 |      | x     |
            | runway  | no    | yes            |                 |                 |      | x     |
            | runway  | no    | no             |                 |                 |      |       |
            | runway  | no    |                | yes             |                 |      | x     |
            | runway  | no    |                | no              |                 |      |       |
            | runway  | no    | no             | yes             |                 |      | x     |
            | runway  | no    | yes            | no              |                 |      |       |
            | runway  | no    |                |                 | yes             |      | x     |
            | runway  | no    |                |                 | no              |      |       |
            | runway  | no    | no             |                 | yes             |      | x     |
            | runway  | no    | yes            |                 | no              |      |       |
            | runway  | no    |                | no              | yes             |      | x     |
            | runway  | no    |                | yes             | no              |      |       |

    Scenario: Bike - Overwriting implied acccess on ways
        Then routability should be
            | highway  | foot | access | vehicle | bicycle | bothw |
            | cycleway | no   |        |         |         | x     |
            | runway   | no   |        |         |         |       |
            | cycleway | no   | no     |         |         |       |
            | cycleway | no   |        | no      |         |       |
            | cycleway | no   |        |         | no      |       |
            | runway   | no   | yes    |         |         | x     |
            | runway   | no   |        | yes     |         | x     |
            | runway   | no   |        |         | yes     | x     |

    Scenario: Bike - Access tags on ways
        Then routability should be
            | foot | access       | vehicle      | bicycle      | bothw |
            | no   |              |              |              | x     |
            | no   | yes          |              |              | x     |
            | no   | permissive   |              |              | x     |
            | no   | designated   |              |              | x     |
            | no   | some_tag     |              |              | x     |
            | no   | no           |              |              |       |
            | no   | private      |              |              |       |
            | no   | agricultural |              |              |       |
            | no   | forestry     |              |              |       |
            | no   |              | yes          |              | x     |
            | no   |              | permissive   |              | x     |
            | no   |              | designated   |              | x     |
            | no   |              | some_tag     |              | x     |
            | no   |              | no           |              |       |
            | no   |              | private      |              |       |
            | no   |              | agricultural |              |       |
            | no   |              | forestry     |              |       |
            | no   |              |              | yes          | x     |
            | no   |              |              | permissive   | x     |
            | no   |              |              | designated   | x     |
            | no   |              |              | some_tag     | x     |
            | no   |              |              | no           |       |
            | no   |              |              | private      |       |
            | no   |              |              | agricultural |       |
            | no   |              |              | forestry     |       |

    Scenario: Bike - Access tags on both node and way
        Then routability should be
            | foot | access   | node/access | bothw |
            | no   | yes      | yes         | x     |
            | no   | yes      | no          |       |
            | no   | yes      | some_tag    | x     |
            | no   | no       | yes         |       |
            | no   | no       | no          |       |
            | no   | no       | some_tag    |       |
            | no   | some_tag | yes         | x     |
            | no   | some_tag | no          |       |
            | no   | some_tag | some_tag    | x     |

    Scenario: Bike - Access combinations
        Then routability should be
            | highway     | foot | access     | vehicle    | bicycle    | forw | backw |
            | runway      | no   | private    |            | yes        | x    | x     |
            | footway     | no   |            | no         | permissive | x    | x     |
            | motorway    | no   |            |            | yes        | x    |       |
            | track       | no   | forestry   |            | permissive | x    | x     |
            | cycleway    | no   | yes        | designated | no         |      |       |
            | primary     | no   |            | yes        | private    |      |       |
            | residential | no   | permissive |            | no         |      |       |

    Scenario: Bike - Ignore access tags for other modes
        Then routability should be
            | highway  | foot | boat | motor_vehicle | moped | bothw |
            | river    | no   | yes  |               |       |       |
            | cycleway | no   | no   |               |       | x     |
            | runway   | no   |      | yes           |       |       |
            | cycleway | no   |      | no            |       | x     |
            | runway   | no   |      |               | yes   |       |
            | cycleway | no   |      |               | no    | x     |
