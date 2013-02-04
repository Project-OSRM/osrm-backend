@routing @maxspeed @car
Feature: Car - Max speed restrictions

	Background: Use specific speeds
		Given the profile "car"

	Scenario: Car - Respect maxspeeds when lower that way type speed
    	Then routability should be
    	 | highway | maxspeed | bothw    |
    	 | trunk   |          | 9s ~10%  |
    	 | trunk   | 10       | 72s ~10% |

	Scenario: Car - Ignore maxspeed when higher than way speed
    	Then routability should be
    	 | highway     | maxspeed | bothw    |
    	 | residential |          | 29s ~10% |
    	 | residential | 85       | 29s ~10% |

    @todo
  	Scenario: Car - Maxspeed formats
 		Then routability should be
 		 | highway | maxspeed   | bothw    |
 		 | trunk   |            | 9s ~10%  |
 		 | trunk   | 10         | 73s ~10% |
 		 | trunk   | 10mph      | 45s ~10% |
 		 | trunk   | 10 mph     | 45s ~10% |
 		 | trunk   | 10MPH      | 45s ~10% |
 		 | trunk   | 10 MPH     | 45s ~10% |
 		 | trunk   | 10unknown  | 9s ~10%  |
 		 | trunk   | 10 unknown | 9s ~10%  |

    @todo
   	Scenario: Car - Maxspeed special tags
  		Then routability should be
  		 | highway | maxspeed | bothw   |
  		 | trunk   |          | 9s ~10% |
  		 | trunk   | none     | 9s ~10% |
  		 | trunk   | signals  | 9s ~10% |
