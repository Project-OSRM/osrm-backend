@routing @foot @countrymotorroad
Feature:  Testbot - Country motorroad Selection

    # Check that motorroad is avoided
    
    Scenario: foot Profile Selection - motorroad avoided
    
        Given the extract extra arguments "--threads 1"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "foot" initialized with
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
            
        And the ways
            | nodes | highway  | motorroad |
            | ab    | trunk    | yes       |    
            | bc    | trunk    |           |
            | ad    | trunk    |           |
            | de    | trunk    |           |
            | ef    | trunk    |           |  
            | cf    | trunk    |           |
        
        When I route I should get
            | waypoints | route             | status | message                          |
            | a,c       | ad,de,ef,cf,cf    | 200    |                                  |
            | a,b       | ad,de,ef,cf,bc,bc | 200    |                                  |
            
  
  
  
  
  
  
  
  
  
  
  
  
  
  
            
 
