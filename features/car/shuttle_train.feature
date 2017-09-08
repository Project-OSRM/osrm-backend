@routing @car @shuttle_train
Feature: Car - Handle ferryshuttle train routes

    Background:
        Given the profile "car"

    Scenario: Car - Use a shuttle train without duration
        Given the node map
            """
            a b
              :
              c d
            """

        And the ways
            | nodes | highway | route         | motorcar |
            | ab    | primary |               |          |
            | bc    |         | shuttle_train | yes      |
            | cd    | primary |               |          |

        When I route I should get
            | from | to | route       | modes                         |
            | a    | d  | ab,bc,cd,cd | driving,train,driving,driving |
            | d    | a  | cd,bc,ab,ab | driving,train,driving,driving |

    Scenario: Car - Use a shuttle train with duration
        Given the node map
            """
            a b
              :
              c d
            """

        And the ways
            | nodes | highway | route         | motorcar | duration |
            | ab    | primary |               |          |          |
            | bc    |         | shuttle_train | yes      | 00:00:15 |
            | cd    | primary |               |          |          |

        When I route I should get
            | from | to | route       | modes                         |
            | a    | d  | ab,bc,cd,cd | driving,train,driving,driving |
            | d    | a  | cd,bc,ab,ab | driving,train,driving,driving |
