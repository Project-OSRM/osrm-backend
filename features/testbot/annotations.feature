@routing @speed @annotations @turn_penalty
Feature: Annotations

    Background:
        Given the profile "turnbot"
        Given a grid size of 100 meters

    Scenario: Ensure that turn penalties aren't included in annotations
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