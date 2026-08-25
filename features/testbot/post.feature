@routing @post @testbot
Feature: HTTP POST requests

    Background:
        Given the profile "testbot"
        And the node map
            """
            a b
            """

        And the ways
            | nodes |
            | ab    |

    Scenario: POST to an unknown service
        When I POST to /nonsense/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]]}
            """
        Then status code should be InvalidService

    Scenario: POST to an unknown service version
        When I POST to /route/v2/testbot with body
            """
            {"coordinates": [[1,1],[1,1]]}
            """
        Then status code should be InvalidVersion

    Scenario: Services without POST support report NotImplemented
        When I POST to /trip/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]]}
            """
        Then status code should be NotImplemented

        When I POST to /tile/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]]}
            """
        Then status code should be NotImplemented

    Scenario: POST with a body that is not a valid request
        When I POST to /route/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]], "overview": "sometimes"}
            """
        Then status code should be InvalidQuery

        When I POST to /match/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]], "gaps": "sometimes"}
            """
        Then status code should be InvalidQuery

        When I POST to /table/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]], "scale_factor": "big"}
            """
        Then status code should be InvalidQuery

    Scenario: POST with options that do not validate
        When I POST to /table/v1/testbot with body
            """
            {"coordinates": [[1,1]]}
            """
        Then status code should be InvalidOptions

    Scenario: POST with flatbuffers output
        When I POST to /route/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]], "format": "flatbuffers"}
            """
        Then response should be flatbuffers

        When I POST to /table/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]], "format": "flatbuffers"}
            """
        Then response should be flatbuffers

        When I POST to /match/v1/testbot with body
            """
            {"coordinates": [[1,1],[1,1]], "format": "flatbuffers"}
            """
        Then response should be flatbuffers

        When I POST to /nearest/v1/testbot with body
            """
            {"coordinates": [[1,1]], "format": "flatbuffers"}
            """
        Then response should be flatbuffers
