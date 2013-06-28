@routing @testbot @bug @todo
Feature: Testbot - Things that looks like bugs

	Background:
		Given the profile "testbot"
    
    Scenario: Testbot - Triangle problem
    	Given the node map
    	 |   |   |   | d |
    	 | a | b | c |   |
    	 |   |   |   | e |

    	And the ways
    	 | nodes | highway | oneway |
    	 | abc   | primary |        |
    	 | cd    | primary | yes    |
    	 | ce    | river   |        |
    	 | de    | primary |        |

    	When I route I should get
    	 | from | to | route     |
    	 | d    | c  | de,ce     |
    	 | e    | d  | de        |
