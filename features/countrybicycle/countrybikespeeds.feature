@routing @countryfoot @countrybikespeeds
Feature:  Testbot - Country Profile Selection

    # Check that country data is being selected
    # CHE (a) does not support trunk on bike
    # FIN (c) does not support trunk on bike
    # GRC (m) and IRL (o) support bridleway
    # LIU (s) is not a country in list so looks like worldwide
    # so
    # GRC (m) and LIU (s) support trunk
    
    Scenario: Country Profile Selection - highway chosen for country
    
        Given the extract extra arguments "--location-dependent-data data/countrytest.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "countrybicycle" initialized with
        """
        profile.uselocationtags.countryspeeds = true
        """
        
        And the node locations
            | node | lat        | lon      | 
            | a    | 9.5        | 5.0      |
            | b    | 9.5        | 10.0     |
            | c    | 9.5        | 15.0     |
            | d    | 9.0        | 5.0      |
            | e    | 9.0        | 10.0     |
            | f    | 9.0        | 15.0     |
            | g    | 8.5        | 5.0      |
            | h    | 8.5        | 10.0     |
            | i    | 8.5        | 15.0     |
            | j    | 8.0        | 5.0      |
            | k    | 8.0        | 10.0     |
            | l    | 8.0        | 15.0     |
            | m    | 7.5        | 5.0      |
            | n    | 7.5        | 10.0     |
            | o    | 7.5        | 15.0     |
            | p    | 7.0        | 5.0      |
            | q    | 7.0        | 10.0     |
            | r    | 7.0        | 15.0     |
            | s    | 6.5        | 5.0      |
            | t    | 6.5        | 10.0     |
            | u    | 6.5        | 15.0     |
            
        And the ways
            | nodes | highway     | comment
            | ab    | trunk       | CHE - FIN no
            | bc    | trunk       |
            | cf    | trunk       | FIN - BEL no
            | fi    | trunk       | 
            | fl    | trunk       | 
            | or    | trunk       | IRL - world yes
            | ru    | trunk       |
            | mn    | bridleway   | GRC - IRL yes
            | no    | bridleway   |
            | mp    | trunk       | GRC - liu yes
            | ps    | trunk       |

        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        
        When I route I should get
            | waypoints | route          | status | message                          |
            | a,b,c     |                | 400    | Impossible route between points  |
            | c,l       |                | 400    | Impossible route between points  |
            | g,i       |                | 400    | Impossible route between points  |
            | o,u       | or,ru,ru       | 200    |    |
            | m,o       | mn,no,no       | 200    |    | 
            | m,s       | mp,ps,ps       | 200    |    | 
            
  
  
  
  
  
  
  
  
  
  
  
  
  
  
            
 
