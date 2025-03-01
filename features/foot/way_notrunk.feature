@testbot @way @notrunk
Feature: Testbot - notrunk allowed 

    # Check that Nodes need to be in the geojson file to support trunk access.
    # Use the default geopoint around 0.0.
    # This covers both trunk allowed notrunk allowed and no motorroad
    
     Scenario: foot trunk allowed - notrunk failure original behavior

        Given the profile "foot"
        And the extract extra arguments "--threads 1"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the node locations
        # f through o should all fail 
        
            | node | lat        | lon      |
            | a    | 48.65729   | 22.26471 |
            | b    | 48.65648   | 22.26486 |
            | c    | 48.65503   | 22.26521 |
            | d    | 48.65489   | 22.26520 |
            | e    | 48.65426   | 22.26533 |
            | f    | 48.65277   | 22.26556 |
            | g    | 48.65026   | 22.26606 |
            | h    | 48.64937   | 22.26618 |
            | i    | 48.64858   | 22.26634 |
            | j    | 48.64763   | 22.26652 |
            | k    | 48.64730   | 22.26658 |
            | l    | 48.64616   | 22.26681 |
            | m    | 48.64599   | 22.26685 |
            | n    | 48.64568   | 22.26690 |
            
            
        And the ways
            | nodes | highway | motorroad |
            | ab    | primary |           | 
            | bc    | primary |           |
            | cd    | primary | yes       |
            | de    | primary |           |
            | ef    | primary |           |
            | fg    | trunk   |           |
            | gh    | trunk   |           |
            | hi    | trunk   |           |
            | ij    | trunk   |           |
            | jk    | trunk   |           |
            | kl    | trunk   |           |
            | lm    | trunk   | yes       |
            | mn    | primary |           |

        When I route I should get
            | from | to | route       | status| message                          | # |
            | a    | c  | ab,bc,bc    | 200   |                                  |   |
            | a    | f  |             | 400   | Impossible route between points  |   |
            | a    | f  |             | 400   | Impossible route between points  |   |
            | d    | f  | de,ef,ef    | 200   |                                  |   |
            | d    | g  | de,ef,ef    | 200   |                                  |   |
            | d    | n  |             | 400   | Impossible route between points  |   |
  
     #
  
     Scenario: foot trunk allowed - trunk ok with no geojson

        Given the extract extra arguments "--threads 1"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "foot" initialized with
        """
        profile.uselocationtags.trunk = true
        """

        And the node locations
        # a through g are in Slovakia - in the no trunk allowed set
        # h is on the edge (i.e. in Ukraine)
        # i through n are in Ukraine
        
            | node | lat        | lon      |
            | a    | 48.65729   | 22.26471 |
            | b    | 48.65648   | 22.26486 |
            | c    | 48.65503   | 22.26521 |
            | d    | 48.65489   | 22.26520 |
            | e    | 48.65426   | 22.26533 |
            | f    | 48.65277   | 22.26556 |
            | g    | 48.65026   | 22.26606 |
            | h    | 48.64937   | 22.26618 |
            | i    | 48.64858   | 22.26634 |
            | j    | 48.64763   | 22.26652 |
            | k    | 48.64730   | 22.26658 |
            | l    | 48.64616   | 22.26681 |
            | m    | 48.64599   | 22.26685 |
            | n    | 48.64568   | 22.26690 |
            
            
        And the ways
            | nodes | highway | motorroad |
            | ab    | primary |           | 
            | bc    | primary |           |
            | cd    | primary | yes       |
            | de    | primary |           |
            | ef    | primary |           |
            | fg    | trunk   |           |
            | gh    | trunk   |           |
            | hi    | trunk   |           |
            | ij    | trunk   |           |
            | jk    | trunk   |           |
            | kl    | trunk   |           |
            | lm    | trunk   | yes       |
            | mn    | primary |           |

        When I route I should get
            | from | to | route       | status| message                          | # |
            | a    | c  | ab,bc,bc    | 200   |                                  |   |
            | a    | f  |             | 400   | Impossible route between points  |   |
            | d    | f  | de,ef,ef    | 200   |                                  |   |
            | d    | g  | de,ef,fg,fg | 200   |                                  |   |
            | e    | n  |             | 400   | Impossible route between points  |   |
            | f    | h  | fg,gh       | 200   |                                  |   |
            | g    | l  | gh,ij,kl,kl | 200   |                                  |   |
            | h    | l  | hi,ij,kl,kl | 200   |                                  |   |
            | i    | l  | ij,kl,kl    | 200   |                                  |   |
            | i    | m  |             | 400   | Impossible route between points  |   |
  
     Scenario: foot trunk allowed - notrunk failure with geojson

        Given the extract extra arguments "--threads 1 --location-dependent-data data/notrunk.geojson"
        And the partition extra arguments "--threads 1"
        And the customize extra arguments "--threads 1"

        And the profile file "foot" initialized with
        """
        profile.uselocationtags.trunk = true
        """
        
        And the node locations
        # a through g are in Slovakia - in the no trunk allowed set
        # h is on the edge (i.e. in Ukraine)
        # i through n are in Ukraine
        
            | node | lat        | lon      |
            | a    | 48.65729   | 22.26471 |
            | b    | 48.65648   | 22.26486 |
            | c    | 48.65503   | 22.26521 |
            | d    | 48.65489   | 22.26520 |
            | e    | 48.65426   | 22.26533 |
            | f    | 48.65277   | 22.26556 |
            | g    | 48.65026   | 22.26606 |
            | h    | 48.64937   | 22.26618 |
            | i    | 48.64858   | 22.26634 |
            | j    | 48.64763   | 22.26652 |
            | k    | 48.64730   | 22.26658 |
            | l    | 48.64616   | 22.26681 |
            | m    | 48.64599   | 22.26685 |
            | n    | 48.64568   | 22.26690 |
            
            
        And the ways
            | nodes | highway | motorroad |
            | ab    | primary |           | 
            | bc    | primary |           |
            | cd    | primary | yes       |
            | de    | primary |           |
            | ef    | primary |           |
            | fg    | trunk   |           |
            | gh    | trunk   |           |
            | hi    | trunk   |           |
            | ij    | trunk   |           |
            | jk    | trunk   |           |
            | kl    | trunk   |           |
            | lm    | trunk   | yes       |
            | mn    | primary |           |

        When I route I should get
            | from | to | route       | status| message                          | # |
            | a    | c  | ab,bc,bc    | 200   |                                  |   |
            | a    | f  |             | 400   | Impossible route between points  |   |
            | a    | f  |             | 400   | Impossible route between points  |   |
            | d    | f  | de,ef,ef    | 200   |                                  |   |
            | d    | g  |             | 400   | Impossible route between points  |   |
            | e    | n  |             | 400   | Impossible route between points  |   |
            | f    | h  |             | 400   | Impossible route between points  |   |
            | g    | l  | hi,ij,kl,kl | 200   |                                  |   |
            | h    | l  | hi,ij,kl,kl | 200   |                                  |   |
            | i    | l  | ij,kl,kl    | 200   |                                  |   |
            | i    | m  |             | 400   | Impossible route between points  |   |
  
