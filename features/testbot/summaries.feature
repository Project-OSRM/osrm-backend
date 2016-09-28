@routing @basic @testbot
Feature: Basic Routing

    Background:
        Given the profile "testbot"
        Given a grid size of 500 meters

    @smallest
    Scenario: Summaries when routing on a simple network
        Given the node map
            | b |   |   | f |
            |   |   |   |   |
            | c | d |   | g |
            |   |   |   |   |
            | a |   | e |   |

        And the ways
            | nodes | name   |
            | acb   | road   |
            | de    | 1 st   |
            | cd    |        |
            | dg    | blvd   |
            | df    | street |

        When I route I should get
            | waypoints | route                               | summary                  |
            | a,e       | road,,1 st,1 st                     | road, 1 st               |
            | a,d,f     | road,,,street,street                | road;street              |
            | a,e,f     | road,,1 st,1 st,1 st,street,street  | road, 1 st;1 st, street  |
