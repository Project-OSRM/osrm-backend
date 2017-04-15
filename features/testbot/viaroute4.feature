@routing @testbot @via @oneway
Feature: Via points

    Background:
        Given the profile "testbot"

    Scenario: Multiple via points with oneways
        Given the node map
            |   | m |   |
            |   | l |   |
            | j | k |   |
            |   | h |   |
            | i | g |   |
            |   | f | e |
            |   |   | d |
            |   | b | c |
            |   | a |   |

        And the ways
            | nodes | oneway |
            | ab    |        |
            | fb    | yes    |
            | bcd   | yes    |
            | def   | yes    |
            | fg    |        |
            | gh    |        |
            | hk    |        |
            | gijk  |        |
            | klm   |        |

        When I route I should get
            | waypoints | route                   |
            | a,d,l,m   | ab,bcd,def,fg,gh,hk,klm |
            | a,f,h,m   | ab,bcd,def,fg,gh,hk,klm |
            | a,c,m     | ab,bcd,def,fg,gh,hk,klm |
            | a,d,h,m   | ab,bcd,def,fg,gh,hk,klm |
            | a,e,h,m   | ab,bcd,def,fg,gh,hk,klm | 
            | a,m       | ab,bcd,def,fg,gh,hk,klm | 
            

