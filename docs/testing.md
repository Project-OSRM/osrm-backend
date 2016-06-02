# Testsuite

OSRM comes with a testsuite containing both unit-tests using the BOOST library and cucucmber.js for scenario driven testing.

## Unit Tests

TODO write a guide on unit tests.

## Cucumber

For a general introduction on cucumber in our testsuite, have a look at [the wiki](https://github.com/Project-OSRM/osrm-backend/wiki/Cucumber-Test-Suite).

This documentation aims to supply a guideline on how to write cucumber tests that test new features introduced into osrm.

### Test the feature

It is often tempting to reduce the test to a path and accompanying instructions. Instructions can and will change over the course of improving guidance.

Instructions should only be used when writing a feature located in `features/guidance`. All other features should avoid using instructions at all.

### Write Tests to Scale

OSRM is a navigation engine. Tests should always consider this background. 

An important implication is the grid size. If tests use a very small grid size, you run into the chance of instructions being omitted.
For example:

```
Background:
    Given the profile "car"
    Given a grid size of 10 meters

Scenario: Testbot - Straight Road
    Given the node map
        | a | b | c | d |

    And the ways
        | nodes | highway |
        | ab    | primary |
        | bc    | primary |
        | cd    | primary |

    When I route I should get
        | from | to | route       |
        | a    | d  | ab,bc,cd,cd |

```

In a navigation engine, the instructions

 - depart east on ab
 - in 10 meters the road name changes to bc
 - in 10 meters the road name changes to cd
 - you arrived at cd
 
would be impossible to announce and not helpful at all.
Since no actual choices exist, the route you get could result in `ab,cd` and simply say `depart` and `arrive`.

To prevent such surprises, always consider the availability of other roads and use grid sizes/road lengths that correspond to actually reasonable scenarios in a road network.

### Use names

If you specify many nodes in close succession to present a specific road geometry, consider using `name` to indicate to OSRM that the segment is a single road.

```
Background:
    Given the profile "car"
    Given a grid size of 10 meters

Scenario: Testbot - Straight Road
    Given the node map
        | a | b | c | d |

    And the ways
        | nodes | highway | name |
        | ab    | primary | road |
        | bc    | primary | road |
        | cd    | primary | road |

    When I route I should get
        | from | to | route     | turns         |
        | a    | d  | road,road | depart,arrive |

```

Guidance guarantees only essential maneuvers. You will always see `depart` and `arrive` as well as all turns that are not obvious.

So the following scenario does not change the instructions

```
Background:
    Given the profile "car"
    Given a grid size of 10 meters

Scenario: Testbot - Straight Road
    Given the node map
        | a | b |
        | d | c |

    And the ways
        | nodes | highway | name |
        | ab    | primary | road |
        | bc    | primary | road |
        | cd    | primary | road |

    When I route I should get
        | from | to | route     | turns         |
        | a    | d  | road,road | depart,arrive |
```

but if we modify it to

```
Background:
    Given the profile "car"
    Given a grid size of 10 meters

Scenario: Testbot - Straight Road
    Given the node map
        | a | b | e |
        | d | c |   |

    And the ways
        | nodes | highway | name |
        | ab    | primary | road |
        | bc    | primary | road |
        | cd    | primary | road |
        | be    | primary | turn |

    When I route I should get
        | from | to | route          | turns                        |
        | a    | d  | road,road,road | depart,continue right,arrive |
```

### Prevent Randomness

Some features in OSRM can result in strange experiences during testcases. To prevent some of these issues, follow the guidelines below.

#### Use Waypoints
Using grid nodes as waypoints offers the chance of unwanted side effects.
OSRM converts the grid into a so called edge-based graph.

```
Scenario: Testbot - Intersection
    Given the node map
        |   | e |   |
        | b | a | d |
        |   | c |   |

    And the ways
        | nodes | highway | oneway |
        | ab    | primary | yes    |
        | ac    | primary | yes    |
        | ad    | primary | yes    |
        | ae    | primary | yes    |
```
Selecting `a` as a `waypoint` results in four possible starting locations. Which one of the routes `a,b`, `a,c`, `a,d`, or `a,e` is found is pure chance and depends on the order in the static `r-tree`.

To guarantee discovery, use:

```
Scenario: Testbot - Intersection
    Given the node map
        |   |   | e |   |   |
        |   |   | 4 |   |   |
        | b | 1 | a | 3 | d |
        |   |   | 2 |   |   |
        |   |   | c |   |   |

    And the ways
        | nodes | highway | oneway |
        | ab    | primary | yes    |
        | ac    | primary | yes    |
        | ad    | primary | yes    |
        | ae    | primary | yes    |
```
And use `1`,`2`,`3`, and `4` as starting waypoints. The routes `1,b`, `2,c`, `3,d`, and `4,e` can all be discovered.

#### Allow For Small Offsets

Whenever you are independent of the start location (see use waypoints), the waypoint chosen as start/end location can still influence distances/durations.

If you are testing for a duration metric, allow for a tiny offset to ensure a passing test in the presence of rounding/snapping issues.

#### Don't Rely on Alternatives
Alternative route discovery is a random feature in itself. The discovery of routes depends on the contraction order of roads and cannot be assumed successful, ever.
