@routing @link @car
Feature: Car - Speed on links
# Check that there's a reasonable ratio between the 
# speed of a way and it's corresponding link type.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Use motorway_link when reasonable
        Given the node map
            |   |   |   |   | k | l |   |
            |   | e | f |   |   |   |   |
            |   |   |   |   |   |   |   |
            | a | g | h | b | m | n | c |
            |   | i | j |   | o | p |   |

        And the ways
            | nodes  | highway       |
            | ag     | motorway      |
            | hcm    | motorway      |
            | nb     | motorway      |
            | gefh   | motorway      |
            | mkln   | motorway      |
            | gijh   | motorway_link |
            | mopn   | motorway_link |

        When I route I should get
            | from | to | route               |
            | a    | b  | ag,gefh,hcm,mopn,nb |

    Scenario: Car - Use trunk_link when reasonable
        Given the node map
            |   |   |   |   | k | l |   |
            |   | e | f |   |   |   |   |
            |   |   |   |   |   |   |   |
            | a | g | h | b | m | n | c |
            |   | i | j |   | o | p |   |

        And the ways
            | nodes  | highway    |
            | ag     | trunk      |
            | hcm    | trunk      |
            | nb     | trunk      |
            | gefh   | trunk      |
            | mkln   | trunk      |
            | gijh   | trunk_link |
            | mopn   | trunk_link |

        When I route I should get
            | from | to | route               |
            | a    | b  | ag,gefh,hcm,mopn,nb |

    Scenario: Car - Use primary_link when reasonable
        Given the node map
            |   |   |   |   | k | l |   |
            |   | e | f |   |   |   |   |
            |   |   |   |   |   |   |   |
            | a | g | h | b | m | n | c |
            |   | i | j |   | o | p |   |

        And the ways
            | nodes  | highway      |
            | ag     | primary      |
            | hcm    | primary      |
            | nb     | primary      |
            | gefh   | primary      |
            | mkln   | primary      |
            | gijh   | primary_link |
            | mopn   | primary_link |

        When I route I should get
            | from | to | route               |
            | a    | b  | ag,gefh,hcm,mopn,nb |

    Scenario: Car - Use secondary_link when reasonable
        Given the node map
            |   |   |   |   | k | l |   |
            |   | e | f |   |   |   |   |
            |   |   |   |   |   |   |   |
            | a | g | h | b | m | n | c |
            |   | i | j |   | o | p |   |

        And the ways
            | nodes  | highway        |
            | ag     | secondary      |
            | hcm    | secondary      |
            | nb     | secondary      |
            | gefh   | secondary      |
            | mkln   | secondary      |
            | gijh   | secondary_link |
            | mopn   | secondary_link |

        When I route I should get
            | from | to | route               |
            | a    | b  | ag,gefh,hcm,mopn,nb |

    Scenario: Car - Use tertiary_link when reasonable
        Given the node map
            |   |   |   |   | k | l |   |
            |   | e | f |   |   |   |   |
            |   |   |   |   |   |   |   |
            | a | g | h | b | m | n | c |
            |   | i | j |   | o | p |   |

        And the ways
            | nodes  | highway       |
            | ag     | tertiary      |
            | hcm    | tertiary      |
            | nb     | tertiary      |
            | gefh   | tertiary      |
            | mkln   | tertiary      |
            | gijh   | tertiary_link |
            | mopn   | tertiary_link |

        When I route I should get
            | from | to | route               |
            | a    | b  | ag,gefh,hcm,mopn,nb |
