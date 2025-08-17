# This file tests basic Cucumber functionality to verify the v12 upgrade is working
Feature: Hello World
  Simple test to verify Cucumber is working correctly

  Scenario: Say hello
    Given I have a hello message
    When I display the message
    Then I should see "Hello World"