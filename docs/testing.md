# Testsuite

OSRM comes with a testsuite containing both unit-tests using the Boost library and cucumber.js for scenario driven testing.

## Unit Tests

For a general introduction on Boost.Test have a look at [its docs](http://www.boost.org/doc/libs/1_60_0/libs/test/doc/html/index.html).

### Separate Test Binaries

Unit tests should be registered according to the sub-project they're in.
If you want to write tests for utility functions, add them to the utility test binary.
See `CMakeLists.txt` in the unit test directory for how to register new unit tests.

### Using Boost.Test Primitives

There is a difference between only reporting a failed condition and aborting the test right at a failed condition.
Have a look at [`BOOST_CHECK` vs `BOOST_REQUIRE`](http://www.boost.org/doc/libs/1_60_0/libs/test/doc/html/boost_test/utf_reference/testing_tool_ref/assertion_boost_level.html).
Instead of manually checking e.g. for equality, less than, if a function throws etc. use their [corresponding Boost.Test primitives](http://www.boost.org/doc/libs/1_60_0/libs/test/doc/html/boost_test/utf_reference/testing_tool_ref.html).

If you use `BOOST_CHECK_EQUAL` you have to implement `operator<<` for your type so that Boost.Test can print mismatches.
If you do not want to do this, define `BOOST_TEST_DONT_PRINT_LOG_VALUE` (and undef it after the check call) or sidestep it with `BOOST_CHECK(fst == snd);`.

### Test Fixture

If you need to test features on a real dataset (think about this twice: prefer cucumber and dataset-independent tests for their reproducibility and minimality), there is a fixed dataset in `test/data`.
This dataset is a small extract and may not even contain all tags or edge cases.
Furthermore this dataset is not in sync with what you see in up-to-date OSM maps or on the demo server.
See the library tests for how to add new dataset dependent tests.

To prepare the test data simply `cd test/data/` and then run `make`.

### Running Tests

To build the unit tests:

```
cd build
cmake ..
make tests
```

You should see the compiled binaries in `build/unit_tests`, you can then run each suite individually:

```
./engine-tests
```

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
        """
        a b c d
        """

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
        """
        a b c d
        """

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
        """
        a b
        d c
        """

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
        """
        a b e
        d c
        """

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

### Test all directions

Modelling a road as roundabout has an implied oneway tag associated with it. In the following case, we can route from `a` to `d` but not from `d` to `a`.
To discover those errors, make sure to check for all allowed directions.

```
Scenario: Enter and Exit mini roundabout with sharp angle   # features/guidance/mini-roundabout.feature:37
    Given the profile "car"                                   # features/step_definitions/data.js:8
    Given a grid size of 10 meters                            # features/step_definitions/data.js:20
    Given the node map                                        # features/step_definitions/data.js:45
        """
        a b
          c d
        """
    And the ways                                              # features/step_definitions/data.js:128
        | nodes | highway         | name |
        | ab    | tertiary        | MySt |
        | bc    | roundabout      |      |
        | cd    | tertiary        | MySt |
    When I route I should get                                 # features/step_definitions/routing.js:4
        | from | to | route     | turns         | #                                               |
        | a    | d  | MySt,MySt | depart,arrive | # suppress multiple enter/exit mini roundabouts |
        | d    | a  | MySt,MySt | depart,arrive | # suppress multiple enter/exit mini roundabouts |
    Tables were not identical:
        |  from |     to |     route     |     turns         |     #
        |     a |      d |     MySt,MySt |     depart,arrive |     # suppress multiple enter/exit mini roundabouts |
        | (-) d |  (-) a | (-) MySt,MySt | (-) depart,arrive | (-) # suppress multiple enter/exit mini roundabouts |
        | (+) d |  (+) a | (+)           | (+)               | (+) # suppress multiple enter/exit mini roundabouts |
```

### Prevent Randomness

Some features in OSRM can result in strange experiences during testcases. To prevent some of these issues, follow the guidelines below.

#### Use Waypoints

Using grid nodes as waypoints offers the chance of unwanted side effects.
OSRM converts the grid into a so called edge-based graph.

```
Scenario: Testbot - Intersection
    Given the node map
        """
          e
        b a d
          c
        """

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
        """
            e
            4
        b 1 a 3 d
            2
            c
        """

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

### Understanding Turn Restrictions

Adding turn restrictions requires the restriction to follow a very specific format.

We specify them in a table with the header `| type | way:from | way:to | node:via | restriction |`.
It is important that turn restrictions require micro segmentation.

Consider the following scenario:
```
Given the node map:
    """
          e
          |
    a - - b - - c
          |
          d
    """

And the ways
    | nodes | oneway |
    | abc   | yes    |
    | ebd   | yes    |

And the relations
    | type        | way:from | way:to | node:via | restriction   |
    | restriction | abc      | ebd    | b        | no_right_turn |
```

The setting looks perfectly fine at first glance. However, it is not well defined.
The forbidden right turn could be either a superfluous addition, forbidding the turn `cb` to `be`, or actually refer to the turn `ab` to `bd` to say that a turn is forbidden here.

To model turn-restrictions correctly and uniquely, we need to split segments that contribute to the restriction into the smallest possible parts.
E.g. the above scenario could correctly be expressed as:

```
Given the node map:
    """
          e
          |
    a - - b - - c
          |
          d
    """

And the ways
    | nodes | oneway | name |
    | ab    | yes    | abc  |
    | bc    | yes    | abc  |
    | eb    | yes    | ebd  |
    | bd    | yes    | ebd  |

And the relations
    | type        | way:from | way:to | node:via | restriction   |
    | restriction | ab       | bd     | b        | no_right_turn |
```

Unless this format is used, OSRM will omit the (then ambiguous) turn restrictions and ignore them.

## My Guidance Tests are Failing - Understanding what you can change

If you change some stuff in guidance, you will easily see tests change their result. E.g. if you change the angles for which we report `right`, then obviously some tests might not report a `direction modifier` named `right` anymore.

This small section will try to guide you in making the correct decisions for changing the behaviour of tests.

The difficulty in guidance tests is that not all items can be translated 1:1 from the ascii art into turn-angles.

The turn-angle calculation tries to find turn angles that would represent perceived turn angles, not the exact angle at the connection.

This is necessary, since connections in OSM are always bound by the paradigm that the way is supposed to be in the middle of the actual road.
For broad streets, you will see stronger angles than the actual turns.

### Don't change the test, change the expected behaviour

If we have a test that looks like this:

```
Given a grid size of 5 m
Given the node map
"""
a - b - - - - - - c
     \  
      d - - - - - e
"""

When I route I should get
 | waypoints | route       | turns                          |
 | a,e       | abc,bde,bde | depart,turn slight right,arrive|
```

And the test reports `turn right` for the route `a->e`, where before it said `slight right`.

If you change the turn angles, obviously you can expect changes in the distinction between `slight right` and `right`.
In such a case it is, of course, reasonable to change the expected route to report `right` instead of `slight right`. You should consider inspecting the actual turn angles at `b` to see if you feel that change is justified.

However, you should never adjust the test itself.
If you look at a failure, the other way around

```
Given a grid size of 5 m
Given the node map
"""
a - b - - - - - - c
     \  
      d - - - - - e
"""

When I route I should get
 | waypoints | route       | turns                   |
 | a,e       | abc,bde,bde | depart,turn right,arrive|
```

where we see a `slight right`, over the expected `right`.
We could be tempted to adjust the grid size (e.g. from `10 m` to `20` meters). 

Such a change would fundamentally alter the tests, though.
Since the part `b-d` is a short offset, when we are looking at a grid of size `5 m`, the angle calculation will try and compensate for this offset.

In this case we would see a very slight turn angle. If your change now reports different turn angles, you can of course change the expected result. But you should not adjust the grid size. The test would be testing turn angles of `180` and `100` degrees, instead of `180` and `160`. 

### Consider Post-Processing Impacts

Some changes you might see could look completely unrelated. To understand the impact of your changes, you can make use of the debugging utilities you can find in `util/debug.hpp` (and potentially other related headers).

If your test is inspecting a series of turns (remember, a turn does not necessarily equals an instruction), you could see interaction with post-processing.
To see the unprocessed turns, you should print the steps at the end of step assembly (`assembleSteps` in `engine/guidance/assemble_steps.hpp`).

If you see unexpected changes, you can consider adding the `locations` field to your test to study what location a turn is reported at.

To study a test without post-processing impacts, you can create a copy of the case on a very large grid (like 2000 meters). In such a grid, `turn collapsing` would be essentially disable.

Sadly, there is no general guideline.

### Use Caution

If in doubt, ask another person. Inspect as much of the data as possible (e.g. print un-collapsed steps, turn angles and so on) and use your best judgement, if the new result seems justified.
