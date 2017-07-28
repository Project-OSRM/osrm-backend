@routing @speed @annotations
Feature: Annotations

    Scenario: Ensure that turn penalties aren't included in annotations
        Given the profile "turnbot"
        Given a grid size of 100 meters
        Given the node map
            """
              h i
            j k l m
            """

        And the query options
          | annotations | duration,speed,weight |

        And the ways
            | nodes | highway     |
            | hk    | residential |
            | il    | residential |
            | jk    | residential |
            | lk    | residential |
            | lm    | residential |

        When I route I should get
            | from | to | route    | a:speed     | a:weight |
            | h    | j  | hk,jk,jk | 6.7:6.7     | 15:15    |
            | i    | m  | il,lm,lm | 6.7:6.7     | 15:15    |
            | j    | m  | jk,lm    | 6.7:6.7:6.7 | 15:15:15 |


    Scenario: There should be different forward/reverse datasources
        Given the profile "testbot"

        And the node map
            """
            a b c d e f g h i
            """

        And the ways
            | nodes     | highway |
            | abcdefghi | primary |

        And the contract extra arguments "--segment-speed-file {speeds_file}"
        And the customize extra arguments "--segment-speed-file {speeds_file}"

        # Note: 180km/h == 50m/s for speed annotations
        And the speed file
        """
        1,2,180,1
        2,1,180,1
        3,4,180,1
        5,6,180,1
        8,7,180,1
        """
        And the query options
          | annotations | datasources,speed |

        When I route I should get
          | from | to | route               | a:datasources   | a:speed                 |
          | a    | i  | abcdefghi,abcdefghi | 1:0:1:0:1:0:0:0 | 50:10:50:10:50:10:10:10 |
          | i    | a  | abcdefghi,abcdefghi | 0:1:0:0:0:0:0:1 | 10:50:10:10:10:10:10:50 |
