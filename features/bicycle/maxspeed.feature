@routing @maxspeed @bicycle
Feature: Bike - Max speed restrictions

	Background: Use specific speeds
		Given the profile "bicycle"

	Scenario: Bicycle - Respect maxspeeds when lower that way type speed
    	Then routability should be
    	 | highway     | maxspeed | bothw    |
    	 | residential |          | 40s ~10% |
    	 | residential | 10       | 72s ~10% |

	Scenario: Bicycle - Ignore maxspeed when higher than way speed
    	Then routability should be
    	 | highway     | maxspeed | bothw    |
    	 | residential |          | 40s ~10% |
    	 | residential | 80       | 40s ~10% |
    
    @todo
  	Scenario: Bicycle - Maxspeed formats
 		Then routability should be
 		 | highway     | maxspeed  | bothw     |
 		 | residential |           | 40s ~10%  |
 		 | residential | 5         | 144s ~10% |
 		 | residential | 5mph      | 90s ~10%  |
 		 | residential | 5 mph     | 90s ~10%  |
 		 | residential | 5MPH      | 90s ~10%  |
 		 | residential | 5 MPH     | 90s ~10%  |
 		 | trunk       | 5unknown  | 40s ~10%  |
 		 | trunk       | 5 unknown | 40s ~10%  |

    @todo
   	Scenario: Bicycle - Maxspeed special tags
  		Then routability should be
  		 | highway     | maxspeed | bothw    |
  		 | residential |          | 40s ~10% |
  		 | residential | none     | 40s ~10% |
  		 | residential | signals  | 40s ~10% |
