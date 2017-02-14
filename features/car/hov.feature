@routing @car @hov
Feature: Car - Handle driving

    Background:
        Given the profile "car"
        And a grid size of 100 meters

    Scenario: Car - Avoid hov when not on hov
        Given the node map
            """
               b=========c========================e====j
             ~            ~                         ~
            a              ~                         f----m
            |               i                        |
            |               |        ----------------l
            |               |       /
            g_______________h______k_____n
            """

        And the ways
            | nodes | highway       | hov           |
            | ab    | motorway_link |               |
            | bcej  | motorway      | designated    |
            | ag    | primary       |               |
            | ghkn  | primary       |               |
            | ih    | primary       |               |
            | kl    | secondary     |               |
            | lf    | secondary     |               |
            | ci    | motorway_link |               |
            | ef    | motorway_link |               |
            | fm    | secondary     |               |

        When I route I should get
            | from | to | route                |
            | a    | m  | ag,ghkn,kl,lf,fm,fm  |
            | c    | m  | bcej,ef,fm,fm        |
