@testbot @trunkallowed
Feature: Testbot - trunk allowed 

    # Check that Nodes need to be in the geojson file to support trunk access.
    # Use the default geopoint around 0.0.
    # This covers both trunk allowed and trunk allowed no motorroad
    
    Scenario: trunk allowed - failure due to out of country range
    
        Given the extract extra arguments "--location-dependent-data data/trunk_allowed.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat        | lon      | 
            | a    | 0.120327   | 0.731195 |
            | b    | 0.120251   | 0.730932 |
            | c    | 0.120128   | 0.731389 |
            | d    | 0.120065   | 0.730920 |
            
        And the ways
            | nodes | highway |
            | ad    | primary | 
            | bc    | primary |
            | ab    | trunk   |

        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        
        When I route I should get
            | waypoints | route | status | message                          |
            | c,b,a,d   |       | 400    | Impossible route between points  |
            | d,a,b,c   |       | 400    | Impossible route between points  |
            
            
    # Countries that allow trunk access if motorroad is no (or untagged)
    # Use Greece coordinates.  

    Scenario: trunk allowed no motorroad - failure due to geojson not provided
    
    # By default The Bicycle profile does not allow access on highway="trunk"
     
        Given the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat         | lon       | 
            | a    | 35.120327   | 25.731195 |
            | b    | 35.120251   | 25.730932 |
            | c    | 35.120128   | 25.731389 |
            | d    | 35.120065   | 25.730920 |
            
        And the ways
            | nodes | highway |
            | ad    | primary | 
            | bc    | primary |
            | ab    | trunk   |

        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        When I route I should get
            | waypoints | route | status | message                          |
            | c,b,a,d   |       | 400    | Impossible route between points  |
            | d,a,b,c   |       | 400    | Impossible route between points  |
            
    Scenario: trunk allowed no motorroad  - success (includes no motorroad tag)

    # By default The Bicycle profile does not allow access on highway="trunk"

        Given the origin 35.120327,25.731195
        
        And the extract extra arguments "--location-dependent-data data/trunk_allowed.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat         | lon       | 
            | a    | 35.120327   | 25.731195 |
            | b    | 35.120251   | 25.730932 |
            | c    | 35.120128   | 25.731389 |
            | d    | 35.120065   | 25.730920 |
            
        And the ways
            | nodes | highway |
            | ad    | primary | 
            | bc    | primary |
            | ab    | trunk   |

        When I route I should get
            | waypoints | route             |
            | c,b,a,d   | bc,bc,ab,ab,ad,ad |
            | d,a,b,c   | ad,ad,ab,ab,bc,bc |
            
    Scenario: trunk allowed no motorroad  - failure due to motorroad = "yes" 

        Given the origin 35.120327,25.731195
        
        And the extract extra arguments "--location-dependent-data data/trunk_allowed.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat         | lon       | 
            | a    | 35.120327   | 25.731195 |
            | b    | 35.120251   | 25.730932 |
            | c    | 35.120128   | 25.731389 |
            | d    | 35.120065   | 25.730920 |
            
        And the ways
            | nodes | highway | motorroad |
            | ad    | primary | no        |
            | bc    | primary | no        |
            | ab    | trunk   | yes       |

        When I route I should get
            | waypoints | route | status | message                          |
            | c,b,a,d   |       | 400    | Impossible route between points  |
            | d,a,b,c   |       | 400    | Impossible route between points  |
            
    # Countries that allow trunk access
    # Use New Zealans coordinates.  
    
     Scenario: trunk allowed - - failure due to geojson not provided

    # By default The Bicycle profile does not allow access on highway="trunk"

        Given the origin -41.210555,173.395053
        
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat          | lon        | 
            | a    | -41.210555   | 173.395053 |
            | b    | -41.211099   | 173.394173 |
            | c    | -41.211192   | 173.382834 |
            | d    | -41.211997   | 173.382812 |
            
        And the ways
            | nodes | highway |
            | ad    | primary | 
            | bc    | primary |
            | ab    | trunk   |

        When I route I should get
            | waypoints | route | status | message                          |
            | c,b,a,d   |       | 400    | Impossible route between points  |
            | d,a,b,c   |       | 400    | Impossible route between points  |
                       
     Scenario: trunk allowed - success 

    # By default The Bicycle profile does not allow access on highway="trunk"

        Given the origin -41.210555,173.395053
        
        And the extract extra arguments "--location-dependent-data data/trunk_allowed.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat          | lon        | 
            | a    | -41.210555   | 173.395053 |
            | b    | -41.211099   | 173.394173 |
            | c    | -41.211192   | 173.382834 |
            | d    | -41.211997   | 173.382812 |
            
        And the ways
            | nodes | highway |
            | ad    | primary | 
            | bc    | primary |
            | ab    | trunk   |

        When I route I should get
            | waypoints | route             |
            | c,b,a,d   | bc,bc,ab,ab,ad,ad |
            | d,a,b,c   | ad,ad,ab,ab,bc,bc |
            
     Scenario: trunk allowed - success ignoring motorroad

    # By default The Bicycle profile does not allow access on highway="trunk"

        Given the origin -41.210555,173.395053
        
        And the extract extra arguments "--location-dependent-data data/trunk_allowed.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "bicycle" initialized with
        """
        profile.uselocationtags.speeds = true
        """
        
        And the node locations
            | node | lat          | lon        | 
            | a    | -41.210555   | 173.395053 |
            | b    | -41.211099   | 173.394173 |
            | c    | -41.211192   | 173.382834 |
            | d    | -41.211997   | 173.382812 |
            
        And the ways
            | nodes | highway | motorroad |
            | ad    | primary | no        |
            | bc    | primary | no        |
            | ab    | trunk   | yes       |

        When I route I should get
            | waypoints | route             |
            | c,b,a,d   | bc,bc,ab,ab,ad,ad |
            | d,a,b,c   | ad,ad,ab,ab,bc,bc |
            
           
