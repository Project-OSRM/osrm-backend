require 'OSM/StreamParser'

class OSMTestParserCallbacks < OSM::Callbacks
  @@locations = nil
  
  def self.locations
    if @@locations
      @@locations
    else
      #parse the test file, so we can later reference nodes and ways by name in tests 
      @@locations = {}
      file = 'test/data/test.osm'
      callbacks = OSMTestParserCallbacks.new
      parser = OSM::StreamParser.new(:filename => file, :callbacks => callbacks)
      parser.parse
      puts @@locations
    end
  end

  def node(node)
    @@locations[node.name] = [node.lat,node.lon]
  end
end


Given /^the OSM file contains$/ do |string|
  file = 'data/test.osm'
  File.open( file, 'w') {|f| f.write(string) }

  #convert from .osm to .osm.pbf, which is the format osrm reads
  system "osmosis --read-xml data/test.osm --write-pbf data/test.osm.pbf omitmetadata=true"
end

Given /^the speedprofile contains$/ do |string|
  File.open( 'speedprofile.ini', 'w') {|f| f.write(string) }
end



Given /^the data file "([^"]*)" is present$/ do |file|
  File.exists?(file).should == true
end

When /^I run the extractor with "([^"]*)"$/ do |cmd|
  @response = `#{cmd}`
  #Dir.chdir @test_folder do
  #  @response = IO.popen([cmd, :err=>[:child, :out]]) { |ls_io| ls_result_with_error = ls_io.read }
  #end
end

When /^I run the preprocessor with "([^"]*)"$/ do |cmd|
  @response = `#{cmd}`
end

Given /^the preprocessed files for "([^"]*)" are present and up to date$/ do |area|
  File.exists?("#{area}.osrm").should == true
  File.exists?("#{area}.osrm.names").should == true
  File.exists?("#{area}.osrm.restrictions").should == true
  File.exists?("#{area}.osrm.hsgr").should == true
  File.exists?("#{area}.osrm.nodes").should == true
  File.exists?("#{area}.osrm.edges").should == true
  File.exists?("#{area}.osrm.ramIndex").should == true
  File.exists?("#{area}.osrm.fileIndex").should == true
end

Then /^I should see the file "([^"]*)"$/ do |file|
  File.exists?(file).should == true
end

When /^preprocessed files for "([^"]*)" has been removed$/ do |file|
  FileUtils.rm_r  Dir["#{file}.*"], :secure => true
end

