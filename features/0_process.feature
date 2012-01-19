@process
Feature: Preprocessing OpenStreetMap data
	In order to enable efficient routing
	As the OSRM server
	I want to be able to preprocess OpenStreetMap data
	
	Scenario: Processing OpenStreetMap data using bicycle profile
		Given I am in the test folder
		And the data file "data/kbh.osm.pbf" is present
		And the "bicycle" speedprofile is used
 	   	
		When I run the extractor with "./osrm-extract data/kbh.osm.pbf"
		Then the response should include "extracting data from input file data/kbh.osm.pbf"
		And the response should include 'Using profile "bicycle"'
		And the response should include "[extractor] finished"
		And I should see the file "data/kbh.osrm"
		And I should see the file "data/kbh.osrm.names"
		And I should see the file "data/kbh.osrm.restrictions"
		
		When I run the preprocessor with "./osrm-prepare data/kbh.osrm data/kbh.osrm.restrictions"
		Then the response should include "finished preprocessing"
		And I should see the file "data/kbh.osrm.hsgr"
		And I should see the file "data/kbh.osrm.nodes"
		And I should see the file "data/kbh.osrm.ramIndex"
		And I should see the file "data/kbh.osrm.fileIndex"
