@routing @car
Feature: Car - Handle physical limitation

    Background:
        Given the profile "car"

    Scenario: Car - Use a narrow way
        Then routability should be
            | highway | width  | narrow | bothw |
            | primary |        |        | x     |
            | primary | narrow |        | x     |
            | primary |        | yes    | x     |
            | primary | 1.8    |        |       |
            | primary | 1.9    |        |       |
            | primary | 2.0    |        | x     |
            | primary | 2.1    |        | x     |
            | primary | 1m     |        |       |
            | primary | 1 m    |        |       |
            | primary | 3 m    |        | x     |
            | primary | 6'     |        |       |
            | primary | 6'0"   |        |       |
            | primary | 6'2"   |        |       |
            | primary | 6'3"   |        | x     |
            | primary | 7'     |        | x     |
            | primary | 7'0"   |        | x     |

    Scenario: Car - Narrow combined with width
        Then routability should be
            | highway | width | maxwidth | narrow | bothw |
            | primary | 1     |          | yes    |       |
            | primary | 1.9   |          | yes    |       |
            | primary | 2.0   |          | yes    | x     |
            | primary | 3     |          | yes    | x     |
            | primary |       | 1        | yes    |       |
            | primary |       | 3        | yes    | x     |

    Scenario: Car - Narrow boundary at vehicle width
        Given the profile file "car" initialized with
        """
        profile.vehicle_width = 2.2
        """
        Then routability should be
            | highway | width | narrow | bothw |
            | primary | 2.2   |        |       |
            | primary | 2.3   |        | x     |
            | primary |       | yes    |       |

    Scenario: Car - Limited by width
        Then routability should be
            | highway | maxwidth:physical | maxwidth | width | est_width | bothw |
            | primary | 1                 |          |       |           |       |
            | primary | 3                 |          |       |           | x     |
            | primary |                   | 1        |       |           |       |
            | primary |                   | 3        |       |           | x     |
            | primary |                   |          | 1     |           |       |
            | primary |                   |          | 3     |           | x     |
            | primary |                   |          |       | 1         |       |
            | primary |                   |          |       | 3         | x     |
            | primary | 3                 | 1        |       |           |       |
            | primary | 3                 |          | 1     |           |       |
            | primary | 3                 |          |       | 1         |       |
            | primary |                   | 3        | 1     |           |       |
            | primary |                   | 3        |       | 1         |       |
            | primary |                   |          | 3     | 1         |       |
            | primary | 1                 | 3        |       |           |       |
            | primary | 3                 | 3        |       |           | x     |
            | primary | 3                 | 1.9      |       |           |       |
            | primary | 3                 | 2.0      |       |           | x     |

    Scenario: Car - Limited by width, directional multi-tag
        Then routability should be
            | highway | maxwidth:physical | maxwidth | maxwidth:forward | maxwidth:backward | width | width:forward | width:backward | forw | backw |
            | primary | 3                 |          | 1                |                   |       |               |                |      | x     |
            | primary | 3                 |          |                  | 1                 |       |               |                | x    |       |
            | primary |                   | 3        | 1                |                   |       |               |                |      | x     |
            | primary |                   | 3        |                  | 1                 |       |               |                | x    |       |
            | primary | 3                 |          |                  |                   |       | 1             |                |      | x     |
            | primary | 3                 |          |                  |                   |       |               | 1              | x    |       |
            | primary |                   | 1        |                  |                   | 3     |               |                |      |       |
            | primary | 3                 |          |                  |                   | 3     |               |                | x    | x     |

    Scenario: Car - Limited by height
        Then routability should be
            | highway | maxheight:physical | maxheight | bothw |
            | primary |                    |           | x     |
            | primary | 1                  |           |       |
            | primary | 3                  |           | x     |
            | primary |                    | 1         |       |
            | primary |                    | 8'        | x     |
            | primary |                    | 3         | x     |
            | primary |                    | default   | x     |
            | primary |                    | none      | x     |
            | primary |                    | no-sign   | x     |
            | primary |                    | unsigned  | x     |
            | primary | 3                  | 1         |       |
            | primary | 1                  | 3         |       |
            | primary | default            | 1         |       |
            | primary | 3                  | none      | x     |
            | primary | 3                  | unsigned  | x     |
            | primary | default            | none      | x     |
            | primary | 1                  | none      |       |
            | primary | 3                  | 3         | x     |

    Scenario: Car - Limited by height, directional multi-tag
        Then routability should be
            | highway | maxheight:physical | maxheight:physical:forward | maxheight:physical:backward | maxheight | maxheight:forward | maxheight:backward | forw | backw |
            | primary | 3                  |                            |                             |           | 1                 |                    |      | x     |
            | primary | 3                  |                            |                             |           |                   | 1                  | x    |       |
            | primary |                    | 1                          |                             | 3         |                   |                    |      | x     |
            | primary |                    |                            | 1                           | 3         |                   |                    | x    |       |
            | primary |                    | 1                          |                             |           |                   | 1                  |      |       |
            | primary |                    |                            | 1                           |           | 1                 |                    |      |       |

    Scenario: Car - Limited by length
        Then routability should be
            | highway | maxlength | bothw |
            | primary |           | x     |
            | primary | 1         |       |
            | primary | 5         | x     |
            | primary | unsigned  | x     |

    Scenario: Car - Limited by weight
        Then routability should be
            | highway | maxweightrating | maxweight | bothw |
            | primary |                 |           | x     |
            | primary |                 | 1         |       |
            | primary |                 | 2         | x     |
            | primary |                 | 3.5       | x     |
            | primary |                 | 35000 kg  | x     |
            | primary |                 | 8.9t      | x     |
            | primary |                 | 0.1 lbs   |       |
            | primary |                 | unsigned  | x     |
            | primary | 1               |           |       |
            | primary | 2               |           | x     |
            | primary | 3.5             |           | x     |
            | primary | 35000 kg        |           | x     |
            | primary | 8.9t            |           | x     |
            | primary | 0.1 lbs         |           |       |
            | primary | 1               | 2         |       |
            | primary | 1               | 3         |       |
            | primary | 3               | 1         |       |
            | primary | 3               | 3         | x     |

    Scenario: Car - Limited by weight, directional multi-tag
        Then routability should be
            | highway | maxweight | maxweight:forward | maxweight:backward | maxweightrating | maxweightrating:forward | maxweightrating:backward | forw | backw |
            | primary | 3         |                   |                    |                 | 1                       |                          |      | x     |
            | primary | 3         |                   |                    |                 |                         | 1                        | x    |       |
            | primary |           | 1                 |                    | 3               |                         |                          |      | x     |
            | primary |           |                   | 1                  | 3               |                         |                          | x    |       |
            | primary |           | 1                 |                    |                 |                         | 1                        |      |       |
            | primary |           |                   | 1                  |                 | 1                       |                          |      |       |
