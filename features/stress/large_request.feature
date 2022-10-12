@stress
Feature: Large requests
    Background:
        Given the profile "testbot"
    Scenario: Table request with a lot of input coordinates
        Given the node map
            """
            a b c d e f g h i j k l m n o p q r s t u v w x y z A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
            """

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | de    |
            | ef    |
            | fg    |
            | gh    |
            | hi    |
            | ij    |
            | jk    |
            | kl    |
            | lm    |
            | mn    |
            | no    |
            | op    |
            | pq    |
            | qr    |
            | rs    |
            | st    |
            | tu    |
            | uv    |
            | vw    |
            | wx    |
            | xy    |
            | yz    |
            | zA    |
            | AB    |
            | BC    |
            | CD    |
            | DE    |
            | EF    |
            | FG    |
            | GH    |
            | HI    |
            | IJ    |
            | JK    |
            | KL    |
            | LM    |
            | MN    |
            | NO    |
            | OP    |
            | PQ    |
            | QR    |
            | RS    |
            | ST    |
            | TU    |
            | UV    |
            | VW    |
            | WX    |
            | XY    |
            | YZ    |
        When I request a travel distance matrix I should get
            |   |   a   | b     | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q | r | s | t | u | v | w | x | y | z | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O | P | Q | R | S | T | U | V | W | X | Y | Z |
            | f | 0     | 300   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            | g | 300   | 0     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |