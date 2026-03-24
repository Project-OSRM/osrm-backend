@extract @car @lanes
Feature: Car - Lane count handling

    Background:
        Given the profile "car"

    Scenario: Lanes with valid integer values
        Then routability should be
            | highway   | lanes | bothw |
            | primary   | 1     | x     |
            | primary   | 2     | x     |
            | primary   | 4     | x     |
            | secondary | 3     | x     |

    Scenario: Lanes with invalid decimal values should not crash
        Then routability should be
            | highway   | lanes | bothw |
            | primary   | 2.5   | x     |
            | primary   | 3.7   | x     |
            | secondary | 1.2   | x     |
            | tertiary  | 4.9   | x     |

    Scenario: Lanes:forward and lanes:backward with valid values
        Then routability should be
            | highway   | lanes:forward | lanes:backward | bothw |
            | primary   | 2             | 2              | x     |
            | primary   | 1             | 1              | x     |
            | secondary | 3             | 2              | x     |

    Scenario: Lanes:forward and lanes:backward with invalid decimal values should not crash
        Then routability should be
            | highway   | lanes:forward | lanes:backward | bothw |
            | primary   | 2.5           | 2              | x     |
            | primary   | 2             | 1.5            | x     |
            | secondary | 3.7           | 2.3            | x     |

    Scenario: Lanes with zero and negative values should be ignored
        Then routability should be
            | highway   | lanes | bothw |
            | primary   | 0     | x     |
            | primary   | -1    | x     |
            | secondary | -5    | x     |

    Scenario: Lanes with non-numeric values should be ignored
        Then routability should be
            | highway   | lanes | bothw |
            | primary   | abc   | x     |
            | primary   | two   | x     |
            | secondary | 2;3   | x     |
