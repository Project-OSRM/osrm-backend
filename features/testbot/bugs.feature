@routing @testbot @bug
Feature: Known bugs

    Background:
        Given the profile "testbot"
 
@x
    Scenario: Looping
        Given the input file test/input/looping.osm
        
        And the node locations
            | node | lat       | lon       |
            | a    | 55.664913 | 12.601698 |
            | s    | 55.664948 | 12.601887 |
            | t    | 55.665016 | 12.602199 |
            | u    | 55.665302 | 12.603515 |
            | b    | 55.665302 | 12.603517 |
        
        When I route I should get
            | waypoints | route           |
            | a,s,b     | Prags Boulevard |
            | a,t,b     | Prags Boulevard |
            | a,u,b     | Prags Boulevard |
