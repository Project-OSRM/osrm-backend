@routing @countryfoot @access
Feature: Countryfoot - Access tags on ways
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "countryfoot"

    Scenario: Countryfoot - Access tag hierarchy on ways
        Then routability should be
            | highway  | access | foot | bothw |
            | footway  |        |      | x     |
            | footway  |        | yes  | x     |
            | footway  |        | no   |       |
            | footway  | yes    |      | x     |
            | footway  | yes    | yes  | x     |
            | footway  | yes    | no   |       |
            | footway  | no     |      |       |
            | footway  | no     | yes  | x     |
            | footway  | no     | no   |       |
            | motorway |        |      |       |
            | motorway |        | yes  | x     |
            | motorway |        | no   |       |
            | motorway | yes    |      | x     |
            | motorway | yes    | yes  | x     |
            | motorway | yes    | no   |       |
            | motorway | no     |      |       |
            | motorway | no     | yes  | x     |
            | motorway | no     | no   |       |


    Scenario: Countryfoot - Overwriting implied acccess on ways
        Then routability should be
            | highway  | access | foot | bothw |
            | footway  |        |      | x     |
            | motorway |        |      |       |
            | footway  | no     |      |       |
            | footway  |        |      | x     |
            | footway  |        | no   |       |
            | motorway | yes    |      | x     |
            | motorway |        |      |       |
            | motorway |        | yes  | x     |

    Scenario: Countryfoot - Access tags on ways
        Then routability should be
            | access       | foot         | bothw |
            |              |              | x     |
            | yes          |              | x     |
            | permissive   |              | x     |
            | designated   |              | x     |
            | some_tag     |              | x     |
            | no           |              |       |
            | private      |              |       |
            | agricultural |              |       |
            | forestry     |              |       |
            | delivery     |              |       |
            |              | yes          | x     |
            |              | permissive   | x     |
            |              | designated   | x     |
            |              | some_tag     | x     |
            |              | no           |       |
            |              | private      |       |
            |              | agricultural |       |
            |              | forestry     |       |
            |              | delivery     |       |

    Scenario: Countryfoot - Access tags on both node and way
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

    Scenario: Countryfoot - Access combinations
        Then routability should be
            | highway     | access     | foot       | bothw |
            | motorway    | private    | yes        | x     |
            | footway     |            | permissive | x     |
            | track       | forestry   | permissive | x     |
            | footway     | yes        | no         |       |
            | primary     |            | private    |       |
            | residential | permissive | no         |       |

    Scenario: Countryfoot - Ignore access tags for other modes
        Then routability should be
            | highway  | boat | motor_vehicle | moped | bothw |
            | river    | yes  |               |       |       |
            | footway  | no   |               |       | x     |
            | motorway |      | yes           |       |       |
            | footway  |      | no            |       | x     |
            | motorway |      |               | yes   |       |
            | footway  |      |               | no    | x     |

    Scenario: Countryfoot - Bridleways when access is explicit
        Then routability should be
            | highway   | horse      | bicycle | foot | bothw |
            | bridleway |            |         | yes  | x     |
            | bridleway |            | yes     |      |       |
            | bridleway | designated |         |      |       |
            | bridleway |            |         |      |       |

    Scenario: Countryfoot - a way with missing :forward tag
        Then routability should be
            | highway  | bicycle:backward | foot:backward | forw | backw |
            | cycleway | designated       | designated    |      | x     |

    Scenario: Countryfoot - Access combinations
        Then routability should be
            | highway    | access     | bothw |
            | primary    | permissive | x     |
            | steps      | permissive | x     |
            | footway    | permissive | x     |
            | garbagetag | permissive | x     |

