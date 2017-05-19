@routing @bicycle @access
Feature: Bike - Access tags on ways
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Access tag hierarchy on ways
        Then routability should be
            | access | vehicle | bicycle  | bothw        |
            |        |         |          | cycling      |
            |        |         | yes      | cycling      |
            |        |         | no       | pushing bike |
            |        |         | dismount | pushing bike |
            |        | yes     |          | cycling      |
            |        | yes     | yes      | cycling      |
            |        | yes     | no       | pushing bike |
            |        | yes     | dismount | pushing bike |
            |        | no      |          | pushing bike |
            |        | no      | yes      | cycling      |
            |        | no      | no       | pushing bike |
            |        | no      | dismount | pushing bike |
            | yes    |         |          | cycling      |
            | yes    |         | yes      | cycling      |
            | yes    |         | no       | pushing bike |
            | yes    |         | dismount | pushing bike |
            | yes    | yes     |          | cycling      |
            | yes    | yes     | yes      | cycling      |
            | yes    | yes     | no       | pushing bike |
            | yes    | yes     | dismount | pushing bike |
            | yes    | no      |          | pushing bike |
            | yes    | no      | yes      | cycling      |
            | yes    | no      | no       | pushing bike |
            | yes    | no      | dismount | pushing bike |
            | no     |         |          |              |
            | no     |         | yes      | cycling      |
            | no     |         | no       |              |
            | no     |         | dismount | pushing bike |
            | no     | yes     |          | cycling      |
            | no     | yes     | yes      | cycling      |
            | no     | yes     | no       |              |
            | no     | yes     | dismount | pushing bike |
            | no     | no      |          |              |
            | no     | no      | yes      | cycling      |
            | no     | no      | no       |              |
            | no     | no      | dismount | pushing bike |

    Scenario: Bike - Access tag for both bicycle and foot
        Then routability should be
            | access | foot | bicycle  | bothw        | #                        |
            |        |      |          | cycling      |                          |
            |        |      | yes      | cycling      |                          |
            |        |      | no       | pushing bike |                          |
            |        |      | dismount | pushing bike |                          |
            |        | yes  |          | cycling      |                          |
            |        | yes  | yes      | cycling      |                          |
            |        | yes  | no       | pushing bike |                          |
            |        | yes  | dismount | pushing bike |                          |
            |        | no   |          | cycling      |                          |
            |        | no   | yes      | cycling      |                          |
            |        | no   | no       |              |                          |
            |        | no   | dismount | pushing bike | questionable combination |
            | yes    |      |          | cycling      |                          |
            | yes    |      | yes      | cycling      |                          |
            | yes    |      | no       | pushing bike |                          |
            | yes    |      | dismount | pushing bike |                          |
            | yes    | yes  |          | cycling      |                          |
            | yes    | yes  | yes      | cycling      |                          |
            | yes    | yes  | no       | pushing bike |                          |
            | yes    | yes  | dismount | pushing bike |                          |
            | yes    | no   |          | cycling      |                          |
            | yes    | no   | yes      | cycling      |                          |
            | yes    | no   | no       |              |                          |
            | yes    | no   | dismount | pushing bike | questionable combination |
            | no     |      |          |              |                          |
            | no     |      | yes      | cycling      |                          |
            | no     |      | no       |              |                          |
            | no     |      | dismount | pushing bike |                          |
            | no     | yes  |          | pushing bike |                          |
            | no     | yes  | yes      | cycling      |                          |
            | no     | yes  | no       | pushing bike |                          |
            | no     | yes  | dismount | pushing bike |                          |
            | no     | no   |          |              |                          |
            | no     | no   | yes      | cycling      |                          |
            | no     | no   | no       |              |                          |
            | no     | no   | dismount | pushing bike | questionable combination |

    Scenario: Bike - Access tag in forward direction
        Then routability should be
            | access:forward | vehicle:forward | bicycle:forward | forw         | backw   |
            |                |                 |                 | cycling      | cycling |
            | no             |                 |                 |              | cycling |
            | yes            | no              |                 | pushing bike | cycling |
            |                | yes             | no              | pushing bike | cycling |
            | yes            |                 |                 | cycling      | cycling |
            | no             | yes             |                 | cycling      | cycling |
            |                | no              | yes             | cycling      | cycling |

    Scenario: Bike - Access tag in backward direction
        Then routability should be
            | access:backward | vehicle:backward | bicycle:backward | forw    | backw        |
            |                 |                  |                  | cycling | cycling      |
            | no              |                  |                  | cycling |              |
            | yes             | no               |                  | cycling | pushing bike |
            |                 | yes              | no               | cycling | pushing bike |
            | yes             |                  |                  | cycling | cycling      |
            | no              | yes              |                  | cycling | cycling      |
            |                 | no               | yes              | cycling | cycling      |


    Scenario: Bike - Overwriting implied acccess on ways
        Then routability should be
            | highway  | access | vehicle | bicycle | foot | bothw        |
            | cycleway |        |         |         |      | cycling      |
            | runway   |        |         |         |      |              |
            | cycleway | no     |         |         |      |              |
            | cycleway |        | no      |         |      |              |
            | cycleway |        |         | no      |      |              |
            | cycleway |        |         |         | no   | cycling      |
            | runway   | yes    |         |         |      | cycling      |
            | runway   |        | yes     |         |      | cycling      |
            | runway   |        |         | yes     |      | cycling      |
            | runway   |        |         |         | yes  | pushing bike |

    Scenario: Bike - Access tags on ways where you can bike
        Then routability should be
            | highway | access       | bicycle      | foot         | bothw        |
            |         |              |              |              | cycling      |
            | primary | yes          |              |              | cycling      |
            | primary | permissive   |              |              | cycling      |
            | primary | designated   |              |              | cycling      |
            | primary | some_tag     |              |              | cycling      |
            | primary | no           |              |              |              |
            | primary | private      |              |              |              |
            | primary | agricultural |              |              |              |
            | primary | forestry     |              |              |              |
            | primary | delivery     |              |              |              |
            | primary |              | yes          |              | cycling      |
            | primary |              | permissive   |              | cycling      |
            | primary |              | designated   |              | cycling      |
            | primary |              | some_tag     |              | cycling      |
            | primary |              | no           |              | pushing bike |
            | primary |              | private      |              | pushing bike |
            | primary |              | agricultural |              | pushing bike |
            | primary |              | forestry     |              | pushing bike |
            | primary |              | delivery     |              | pushing bike |
            | primary |              |              | yes          | cycling      |
            | primary |              |              | permissive   | cycling      |
            | primary |              |              | designated   | cycling      |
            | primary |              |              | some_tag     | cycling      |
            | primary |              |              | no           | cycling      |
            | primary |              |              | private      | cycling      |
            | primary |              |              | agricultural | cycling      |
            | primary |              |              | forestry     | cycling      |
            | primary |              |              | delivery     | cycling      |
 
    Scenario: Bike - Access tags on ways where you can push bike
        Then routability should be
            | highway | access       | bicycle      | foot         | bothw        |
            |         |              |              |              | cycling      |
            | footway | yes          |              |              | cycling      |
            | footway | permissive   |              |              | cycling      |
            | footway | designated   |              |              | cycling      |
            | footway | some_tag     |              |              | pushing bike |
            | footway | no           |              |              |              |
            | footway | private      |              |              |              |
            | footway | agricultural |              |              |              |
            | footway | forestry     |              |              |              |
            | footway | delivery     |              |              |              |
            | footway |              | yes          |              | cycling      |
            | footway |              | permissive   |              | cycling      |
            | footway |              | designated   |              | cycling      |
            | footway |              | some_tag     |              | pushing bike |
            | footway |              | no           |              | pushing bike |
            | footway |              | private      |              | pushing bike |
            | footway |              | agricultural |              | pushing bike |
            | footway |              | forestry     |              | pushing bike |
            | footway |              | delivery     |              | pushing bike |
            | footway |              |              | yes          | pushing bike |
            | footway |              |              | permissive   | pushing bike |
            | footway |              |              | designated   | pushing bike |
            | footway |              |              | some_tag     | pushing bike |
            | footway |              |              | no           |              |
            | footway |              |              | private      |              |
            | footway |              |              | agricultural |              |
            | footway |              |              | forestry     |              |
            | footway |              |              | delivery     |              |

    Scenario: Bike - Access tags on ways where you can neither bike or push bike
        Then routability should be
            | highway | access       | bicycle      | foot         | bothw        |
            |         |              |              |              | cycling      |
            | runway  | yes          |              |              | cycling      |
            | runway  | permissive   |              |              | cycling      |
            | runway  | designated   |              |              | cycling      |
            | runway  | some_tag     |              |              |              |
            | runway  | no           |              |              |              |
            | runway  | private      |              |              |              |
            | runway  | agricultural |              |              |              |
            | runway  | forestry     |              |              |              |
            | runway  | delivery     |              |              |              |
            | runway  |              | yes          |              | cycling      |
            | runway  |              | permissive   |              | cycling      |
            | runway  |              | designated   |              | cycling      |
            | runway  |              | some_tag     |              |              |
            | runway  |              | no           |              |              |
            | runway  |              | private      |              |              |
            | runway  |              | agricultural |              |              |
            | runway  |              | forestry     |              |              |
            | runway  |              | delivery     |              |              |
            | runway  |              |              | yes          | pushing bike |
            | runway  |              |              | permissive   | pushing bike |
            | runway  |              |              | designated   | pushing bike |
            | runway  |              |              | some_tag     |              |
            | runway  |              |              | no           |              |
            | runway  |              |              | private      |              |
            | runway  |              |              | agricultural |              |
            | runway  |              |              | forestry     |              |
            | runway  |              |              | delivery     |              |

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
            | highway     | access     | vehicle    | bicycle    | forw         | backw        |
            | runway      | private    |            | yes        | cycling      | cycling      |
            | footway     |            | no         | permissive | cycling      | cycling      |
            | motorway    |            |            | yes        | cycling      |              |
            | track       | forestry   |            | permissive | cycling      | cycling      |
            | cycleway    | yes        | designated | no         | pushing bike | pushing bike |
            | primary     |            | yes        | private    | pushing bike | pushing bike |
            | residential | permissive |            | no         | pushing bike | pushing bike |

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

    Scenario: Bike - Access permissive combinations
        Then routability should be
            | highway    | access     | bothw   |
            | primary    | permissive | cycling |
            | steps      | permissive | cycling |
            | footway    | permissive | cycling |
            | garbagetag | permissive | cycling |


    Scenario: Bike - bicycle=no should prevent pushing bike
        Then routability should be
            | highway | oneway | bicycle | forw         | backw        |
            | footway |        |         | pushing bike | pushing bike |
            | footway |        | no      | pushing bike | pushing bike |
            | primary | yes    |         | cycling      | pushing bike |
            | primary | yes    | no      | pushing bike | pushing bike |

    Scenario: Bike - access=no should prevent pushing bike
        Then routability should be
            | highway | oneway | access | forw         | backw        |
            | footway |        |        | pushing bike | pushing bike |
            | footway |        | no     |              |              |
            | primary | yes    |        | cycling      | pushing bike |
            | primary | yes    | no     |              |              |

    Scenario: Bike - vehicle=no should not prevent pushing bike
        Then routability should be
            | highway | oneway | vehicle | forw         | backw        |
            | footway |        |         | pushing bike | pushing bike |
            | footway |        | no      | pushing bike | pushing bike |
            | primary | yes    |         | cycling      | pushing bike |
            | primary | yes    | no      | pushing bike | pushing bike |
