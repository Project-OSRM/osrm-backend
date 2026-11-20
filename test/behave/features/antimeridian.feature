Feature: Routing across the antimeridian
  In order to ensure routes cross the dateline
  As a user
  I want routing to find continuous ways across longitude 180/-180

  Scenario: Simple route crossing the antimeridian
    Given an OSRM dataset with two nodes connected across the antimeridian
    When I request a route from lon:179.9,lat:0 to lon:-179.9,lat:0
    Then the route should include a segment that crosses the antimeridian
