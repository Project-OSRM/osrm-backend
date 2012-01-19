@launch
Feature: Launching OSRM server
	In order to handle routing request
	As a user
	I want to launch the OSRM server
	
Scenario: Launching the OSRM server
	Given I am in the test folder
	And the preprocessed files for "data/kbh" are present and up to date
	When I start the server with "./osrm-routed"
    Then a process called "osrm-routed" should be running
	And I should see "running and waiting for requests" on the terminal
	
