@routing @foot @area
Feature: Foot - The area profile is the foot profile

    profiles/foot_area.lua is profiles/foot.lua with area meshing switched on,
    and nothing else. While it was a separate copy the two drifted: highway=road
    and leisure=track were walkable there but not in foot, which is what
    features/foot/way.feature pins down for foot itself. These scenarios pin the
    same answers to the area profile, so a future copy cannot reintroduce the
    difference unnoticed.

    Background:
        Given the profile "foot_area"

    Scenario: Foot area - highway=road is not routable
        Then routability should be
            | highway     | forw |
            | road        |      |
            | residential | x    |
            | pedestrian  | x    |

    Scenario: Foot area - leisure=track is not routable
        Then routability should be
            | highway | leisure | forw |
            | (nil)   | track   |      |
            | footway | (nil)   | x    |
