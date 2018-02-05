@guidance
Feature: Internal Intersection Model

    Background:
        Given the profile "car"
        Given a grid size of 10 meters

    Scenario: Dual-carriage way intersection
        Given the node map
          """
             a  b
             |  |
          c--d--e--f
             |  |
          g--h--i--j
             |  |
             k  l
          """

        And the ways
          | nodes | oneway | name                  |
          | adhk  | yes    | Broken Land Parkway   |
          | lieb  | yes    | Broken Land Parkway   |
          | fed   | yes    | Snowden River Parkway |
          | dc    | yes    | Patuxent Woods Drive  |
          | gh    | yes    | Patuxent Woods Drive  |
          | hij   | yes    | Snowden River Parkway |

        When I route I should get
          | waypoints | route                                                              | turns                        | # |
          | a,k       | Broken Land Parkway,Broken Land Parkway                            | depart,arrive                ||
          | l,b       | Broken Land Parkway,Broken Land Parkway                            | depart,arrive                ||
#         | g,j       | Patuxent Woods Drive,Snowden River Parkway,Snowden River Parkway   | depart,continue,arrive       | did not work as expected - might be another issue to handle in post process? |
#         | f,c       | Snowden River Parkway,Patuxent Woods Drive,Patuxent Woods Drive    | depart,continue,arrive       | did not work as expected - might be another issue to handle in post process? |
          | a,c       | Broken Land Parkway,Patuxent Woods Drive,Patuxent Woods Drive      | depart,turn right,arrive     ||
          | g,k       | Patuxent Woods Drive,Broken Land Parkway,Broken Land Parkway       | depart,turn right,arrive     ||
          | l,j       | Broken Land Parkway,Snowden River Parkway,Snowden River Parkway    | depart,turn right,arrive     ||
          | f,b       | Snowden River Parkway,Broken Land Parkway,Broken Land Parkway      | depart,turn right,arrive     ||
          | a,j       | Broken Land Parkway,Snowden River Parkway,Snowden River Parkway    | depart,turn left,arrive      ||
          | g,b       | Patuxent Woods Drive,Broken Land Parkway,Broken Land Parkway       | depart,turn left,arrive      ||
          | l,c       | Broken Land Parkway,Patuxent Woods Drive,Patuxent Woods Drive      | depart,turn left,arrive      ||
          | f,k       | Snowden River Parkway,Broken Land Parkway,Broken Land Parkway      | depart,turn left,arrive      ||
          | a,b       | Broken Land Parkway,Broken Land Parkway,Broken Land Parkway        | depart,continue uturn,arrive ||
          | g,c       | Patuxent Woods Drive,Patuxent Woods Drive,Patuxent Woods Drive     | depart,continue uturn,arrive ||
          | l,k       | Broken Land Parkway,Broken Land Parkway,Broken Land Parkway        | depart,continue uturn,arrive ||
          | f,j       | Snowden River Parkway,Snowden River Parkway,Snowden River Parkway  | depart,continue uturn,arrive ||
